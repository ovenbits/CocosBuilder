/*******************************************************************************
 * Copyright (c) 2013, Esoteric Software
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 * 
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 ******************************************************************************/

#include "SkeletonJson.h"
#include <math.h>
#include <stdio.h>
#include <math.h>
#include "Json.h"
#include "extension.h"
#include "RegionAttachment.h"
#include "AtlasAttachmentLoader.h"

typedef struct {
	SkeletonJson super;
	int ownsLoader;
} _Internal;

SkeletonJson* SkeletonJson_createWithLoader (AttachmentLoader* attachmentLoader) {
	SkeletonJson* self = SUPER(NEW(_Internal));
	self->scale = 1;
	self->attachmentLoader = attachmentLoader;
	return self;
}

SkeletonJson* SkeletonJson_create (Atlas* atlas) {
	AtlasAttachmentLoader* attachmentLoader = AtlasAttachmentLoader_create(atlas);
	SkeletonJson* self = SkeletonJson_createWithLoader(SUPER(attachmentLoader));
	SUB_CAST(_Internal, self) ->ownsLoader = 1;
	return self;
}

void SkeletonJson_dispose (SkeletonJson* self) {
	if (SUB_CAST(_Internal, self) ->ownsLoader) AttachmentLoader_dispose(self->attachmentLoader);
	FREE(self->error);
	FREE(self);
}

void _SkeletonJson_setError (SkeletonJson* self, Json* root, const char* value1, const char* value2) {
	char message[256];
	int length;
	FREE(self->error);
	strcpy(message, value1);
	length = strlen(value1);
	if (value2) strncat(message + length, value2, 256 - length);
	MALLOC_STR(self->error, message);
	if (root) Json_dispose(root);
}

static float toColor (const char* value, int index) {
	char digits[3];
	char *error;
	int color;

	if (strlen(value) != 8) return -1;
	value += index * 2;
	
	digits[0] = *value;
	digits[1] = *(value + 1);
	digits[2] = '\0';
	color = strtoul(digits, &error, 16);
	if (*error != 0) return -1;
	return color / (float)255;
}

static void readCurve (CurveTimeline* timeline, int frameIndex, Json* frame) {
	Json* curve = Json_getItem(frame, "curve");
	if (!curve) return;
	if (curve->type == Json_String && strcmp(curve->valuestring, "stepped") == 0)
		CurveTimeline_setStepped(timeline, frameIndex);
	else if (curve->type == Json_Array) {
		CurveTimeline_setCurve(timeline, frameIndex, Json_getItemAt(curve, 0)->valuefloat, Json_getItemAt(curve, 1)->valuefloat,
				Json_getItemAt(curve, 2)->valuefloat, Json_getItemAt(curve, 3)->valuefloat);
	}
}

static Animation* _SkeletonJson_readAnimation (SkeletonJson* self, Json* root, SkeletonData *skeletonData) {
	Animation* animation;

	Json* bones = Json_getItem(root, "bones");
	int boneCount = bones ? Json_getSize(bones) : 0;

	Json* slots = Json_getItem(root, "slots");
	int slotCount = slots ? Json_getSize(slots) : 0;
    
    Json* drawOrdersMap = Json_getItem(root, "draworder");                  // OVENBITS
    int drawOrderCount = drawOrdersMap ? Json_getSize(drawOrdersMap) : 0;   // OVENBITS

	int timelineCount = 0;
	int i, ii, iii;
	for (i = 0; i < boneCount; ++i)
		timelineCount += Json_getSize(Json_getItemAt(bones, i));
	for (i = 0; i < slotCount; ++i)
		timelineCount += Json_getSize(Json_getItemAt(slots, i));
    timelineCount += drawOrderCount;    // OVENBITS
	animation = Animation_create(root->name, timelineCount);
	animation->timelineCount = 0;
	skeletonData->animations[skeletonData->animationCount] = animation;
	skeletonData->animationCount++;

    for (i = 0; i < boneCount; ++i) {
        int timelineCount;
        Json* boneMap = Json_getItemAt(bones, i);

        const char* boneName = boneMap->name;

        int boneIndex = SkeletonData_findBoneIndex(skeletonData, boneName);
        if (boneIndex == -1) {
            Animation_dispose(animation);
            _SkeletonJson_setError(self, root, "Bone not found: ", boneName);
            return 0;
        }

        timelineCount = Json_getSize(boneMap);
        for (ii = 0; ii < timelineCount; ++ii) {
            float duration;
            Json* timelineArray = Json_getItemAt(boneMap, ii);
            int frameCount = Json_getSize(timelineArray);
            const char* timelineType = timelineArray->name;

            if (strcmp(timelineType, "rotate") == 0) {

                RotateTimeline *timeline = RotateTimeline_create(frameCount);
                timeline->boneIndex = boneIndex;
                for (iii = 0; iii < frameCount; ++iii) {
                    Json* frame = Json_getItemAt(timelineArray, iii);
                    RotateTimeline_setFrame(timeline, iii, Json_getFloat(frame, "time", 0), Json_getFloat(frame, "angle", 0));
                    readCurve(SUPER(timeline), iii, frame);
                }
                animation->timelines[animation->timelineCount++] = (Timeline*)timeline;
                duration = timeline->frames[frameCount * 2 - 2];
                if (duration > animation->duration) animation->duration = duration;

            } else {
                int isScale = strcmp(timelineType, "scale") == 0;
                if (isScale || strcmp(timelineType, "translate") == 0) {
                    float scale = isScale ? 1 : self->scale;
                    TranslateTimeline *timeline = isScale ? ScaleTimeline_create(frameCount) : TranslateTimeline_create(frameCount);
                    timeline->boneIndex = boneIndex;
                    for (iii = 0; iii < frameCount; ++iii) {
                        Json* frame = Json_getItemAt(timelineArray, iii);
                        TranslateTimeline_setFrame(timeline, iii, Json_getFloat(frame, "time", 0), Json_getFloat(frame, "x", 0) * scale,
                                Json_getFloat(frame, "y", 0) * scale);
                        readCurve(SUPER(timeline), iii, frame);
                    }
                    animation->timelines[animation->timelineCount++] = (Timeline*)timeline;
                    duration = timeline->frames[frameCount * 3 - 3];
                    if (duration > animation->duration) animation->duration = duration;
                } else {
                    Animation_dispose(animation);
                    _SkeletonJson_setError(self, 0, "Invalid timeline type for a bone: ", timelineType);
                    return 0;
                }
            }
        }
    }

	for (i = 0; i < slotCount; ++i) {
		int timelineCount;
		Json* slotMap = Json_getItemAt(slots, i);
		const char* slotName = slotMap->name;

		int slotIndex = SkeletonData_findSlotIndex(skeletonData, slotName);
		if (slotIndex == -1) {
			Animation_dispose(animation);
			_SkeletonJson_setError(self, root, "Slot not found: ", slotName);
			return 0;
		}

		timelineCount = Json_getSize(slotMap);
		for (ii = 0; ii < timelineCount; ++ii) {
			float duration;
			Json* timelineArray = Json_getItemAt(slotMap, ii);
			int frameCount = Json_getSize(timelineArray);
			const char* timelineType = timelineArray->name;

			if (strcmp(timelineType, "color") == 0) {
				ColorTimeline *timeline = ColorTimeline_create(frameCount);
				timeline->slotIndex = slotIndex;
				for (iii = 0; iii < frameCount; ++iii) {
					Json* frame = Json_getItemAt(timelineArray, iii);
					const char* s = Json_getString(frame, "color", 0);
					ColorTimeline_setFrame(timeline, iii, Json_getFloat(frame, "time", 0), toColor(s, 0), toColor(s, 1), toColor(s, 2),
							toColor(s, 3));
					readCurve(SUPER(timeline), iii, frame);
				}
				animation->timelines[animation->timelineCount++] = (Timeline*)timeline;
				duration = timeline->frames[frameCount * 5 - 5];
				if (duration > animation->duration) animation->duration = duration;

			} else if (strcmp(timelineType, "attachment") == 0) {
				AttachmentTimeline *timeline = AttachmentTimeline_create(frameCount);
				timeline->slotIndex = slotIndex;
				for (iii = 0; iii < frameCount; ++iii) {
					Json* frame = Json_getItemAt(timelineArray, iii);
					Json* name = Json_getItem(frame, "name");
					AttachmentTimeline_setFrame(timeline, iii, Json_getFloat(frame, "time", 0),
							name->type == Json_NULL ? 0 : name->valuestring);
				}
				animation->timelines[animation->timelineCount++] = (Timeline*)timeline;
				duration = timeline->frames[frameCount - 1];
				if (duration > animation->duration) animation->duration = duration;

			} else {
				Animation_dispose(animation);
				_SkeletonJson_setError(self, 0, "Invalid timeline type for a slot: ", timelineType);
				return 0;
			}
		}
	}
    
    // BEGIN: OVENBITS
    if (drawOrderCount)
    {
        float duration;
        int frameIndex = 0;
        int slotCount = skeletonData->slotCount;
        DrawOrderTimeline *timeline = DrawOrderTimeline_create(drawOrderCount);
        
        for (i = 0; i < drawOrderCount; ++i) {
            
            Json* drawOrderMap = Json_getItemAt(drawOrdersMap, i);
            Json* offsetsMap = Json_getItem(drawOrderMap, "offsets");
            
            int* drawOrder = 0;
            
            if (offsetsMap) {
                int offsetsCount = Json_getSize(offsetsMap);
                
                drawOrder = MALLOC(int, slotCount);
                for (ii = slotCount-1; ii >= 0; ii--)
                    drawOrder[ii] = -1;
                
                int unchanged[slotCount - offsetsCount];
                int originalIndex = 0, unchangedIndex = 0;
                
                int changed[offsetsCount];
                
                for (iii = 0; iii < offsetsCount; ++iii) {
                    Json* offsetMap = Json_getItemAt(offsetsMap, iii);
                    const char* slotName = Json_getString(offsetMap, "slot", "");
                    int slotIndex = SkeletonData_findSlotIndex(skeletonData, slotName);
                    if (slotIndex == -1) {
                        Animation_dispose(animation);
                        _SkeletonJson_setError(self, 0, "Slot not found: ", slotName);
                        return 0;
                    }
                    
                    while (originalIndex != slotIndex)
                        unchanged[unchangedIndex++] = originalIndex++;
                    
                    changed[iii] = slotIndex;
                    
                    drawOrder[originalIndex + Json_getInt(offsetMap, "offset", 0)] = slotIndex;
                    originalIndex++;
                    
                }
                
                originalIndex = 0;
                unchangedIndex = 0;
                while (originalIndex < slotCount) {
                    int flag = 0;
                    for (iii = 0; iii < offsetsCount; iii++)
                    {
                        if (originalIndex == changed[iii])
                            flag = 1;
                    }
                    if (flag != 1) {
                        unchanged[unchangedIndex++] = originalIndex;
                    }
                    originalIndex++;
                }
                
                for (iii = slotCount - 1; iii >= 0; iii--) {
                    if (drawOrder[iii] == -1)
                    {
                        drawOrder[iii] = unchanged[--unchangedIndex];
                    }
                }
            }
            
            DrawOrderTimeline_setFrame(timeline, frameIndex++, Json_getFloat(drawOrderMap, "time", 0), drawOrder,slotCount);
        }
        animation->timelines[animation->timelineCount++] = (Timeline*)timeline;
        duration = timeline->frames[drawOrderCount - 1];
        if (duration > animation->duration) animation->duration = duration;
    }
    // END: OVENBITS

	return animation;
}

SkeletonData* SkeletonJson_readSkeletonDataFile (SkeletonJson* self, const char* path) {
	int length;
	SkeletonData* skeletonData;
	const char* json = _Util_readFile(path, &length);
	if (!json) {
		_SkeletonJson_setError(self, 0, "Unable to read skeleton file: ", path);
		return 0;
	}
	skeletonData = SkeletonJson_readSkeletonData(self, json);
	FREE(json);
	return skeletonData;
}

SkeletonData* SkeletonJson_readSkeletonData (SkeletonJson* self, const char* json) {
	SkeletonData* skeletonData;
	Json *root, *bones;
	int i, ii, iii, boneCount;
	Json* slots;
	Json* skinsMap;
	Json* animations;

	FREE(self->error);
	CONST_CAST(char*, self->error) = 0;

	root = Json_create(json);
	if (!root) {
		_SkeletonJson_setError(self, 0, "Invalid skeleton JSON: ", Json_getError());
		return 0;
	}

	skeletonData = SkeletonData_create();

	bones = Json_getItem(root, "bones");
    if (!bones)
        return 0;
	boneCount = Json_getSize(bones);
	skeletonData->bones = MALLOC(BoneData*, boneCount);
	for (i = 0; i < boneCount; ++i) {
		Json* boneMap = Json_getItemAt(bones, i);
		BoneData* boneData;

		const char* boneName = Json_getString(boneMap, "name", 0);

		BoneData* parent = 0;
		const char* parentName = Json_getString(boneMap, "parent", 0);
		if (parentName) {
			parent = SkeletonData_findBone(skeletonData, parentName);
			if (!parent) {
				SkeletonData_dispose(skeletonData);
				_SkeletonJson_setError(self, root, "Parent bone not found: ", parentName);
				return 0;
			}
		}

		boneData = BoneData_create(boneName, parent);
		boneData->length = Json_getFloat(boneMap, "length", 0) * self->scale;
		boneData->x = Json_getFloat(boneMap, "x", 0) * self->scale;
		boneData->y = Json_getFloat(boneMap, "y", 0) * self->scale;
		boneData->rotation = Json_getFloat(boneMap, "rotation", 0);
		boneData->scaleX = Json_getFloat(boneMap, "scaleX", 1);
		boneData->scaleY = Json_getFloat(boneMap, "scaleY", 1);
        boneData->inheritScale = Json_getInt(boneMap, "inheritScale", 1);
        boneData->inheritRotation = Json_getInt(boneMap, "inheritRotation", 1);

		skeletonData->bones[i] = boneData;
		skeletonData->boneCount++;
	}

	slots = Json_getItem(root, "slots");
	if (slots) {
		int slotCount = Json_getSize(slots);
		skeletonData->slots = MALLOC(SlotData*, slotCount);
		for (i = 0; i < slotCount; ++i) {
			SlotData* slotData;
			const char* color;
			Json *attachmentItem;
			Json* slotMap = Json_getItemAt(slots, i);

			const char* slotName = Json_getString(slotMap, "name", 0);

			const char* boneName = Json_getString(slotMap, "bone", 0);
			BoneData* boneData = SkeletonData_findBone(skeletonData, boneName);
			if (!boneData) {
				SkeletonData_dispose(skeletonData);
				_SkeletonJson_setError(self, root, "Slot bone not found: ", boneName);
				return 0;
			}

			slotData = SlotData_create(slotName, boneData);

			color = Json_getString(slotMap, "color", 0);
			if (color) {
				slotData->r = toColor(color, 0);
				slotData->g = toColor(color, 1);
				slotData->b = toColor(color, 2);
				slotData->a = toColor(color, 3);
			}

			attachmentItem = Json_getItem(slotMap, "attachment");
			if (attachmentItem) SlotData_setAttachmentName(slotData, attachmentItem->valuestring);

			skeletonData->slots[i] = slotData;
			skeletonData->slotCount++;
		}
	}

	skinsMap = Json_getItem(root, "skins");
	if (skinsMap) {
		int skinCount = Json_getSize(skinsMap);
		skeletonData->skins = MALLOC(Skin*, skinCount);
		for (i = 0; i < skinCount; ++i) {
			Json* slotMap = Json_getItemAt(skinsMap, i);
			const char* skinName = slotMap->name;
			Skin *skin = Skin_create(skinName);
			int slotNameCount;

			skeletonData->skins[i] = skin;
			skeletonData->skinCount++;
			if (strcmp(skinName, "default") == 0) skeletonData->defaultSkin = skin;

			slotNameCount = Json_getSize(slotMap);
			for (ii = 0; ii < slotNameCount; ++ii) {
				Json* attachmentsMap = Json_getItemAt(slotMap, ii);
				const char* slotName = attachmentsMap->name;
				int slotIndex = SkeletonData_findSlotIndex(skeletonData, slotName);

				int attachmentCount = Json_getSize(attachmentsMap);
				for (iii = 0; iii < attachmentCount; ++iii) {
					Attachment* attachment;
					Json* attachmentMap = Json_getItemAt(attachmentsMap, iii);
					const char* skinAttachmentName = attachmentMap->name;
					const char* attachmentName = Json_getString(attachmentMap, "name", skinAttachmentName);

					const char* typeString = Json_getString(attachmentMap, "type", "region");
					AttachmentType type;
					if (strcmp(typeString, "region") == 0)
						type = ATTACHMENT_REGION;
					else if (strcmp(typeString, "regionSequence") == 0)
						type = ATTACHMENT_REGION_SEQUENCE;
					else {
						SkeletonData_dispose(skeletonData);
						_SkeletonJson_setError(self, root, "Unknown attachment type: ", typeString);
						return 0;
					}

					attachment = AttachmentLoader_newAttachment(self->attachmentLoader, skin, type, attachmentName);
					if (!attachment) {
						if (self->attachmentLoader->error1) {
							SkeletonData_dispose(skeletonData);
							_SkeletonJson_setError(self, root, self->attachmentLoader->error1, self->attachmentLoader->error2);
							return 0;
						}
						continue;
					}

					if (attachment->type == ATTACHMENT_REGION || attachment->type == ATTACHMENT_REGION_SEQUENCE) {
						RegionAttachment* regionAttachment = (RegionAttachment*)attachment;
						regionAttachment->x = Json_getFloat(attachmentMap, "x", 0) * self->scale;
						regionAttachment->y = Json_getFloat(attachmentMap, "y", 0) * self->scale;
						regionAttachment->scaleX = Json_getFloat(attachmentMap, "scaleX", 1);
						regionAttachment->scaleY = Json_getFloat(attachmentMap, "scaleY", 1);
						regionAttachment->rotation = Json_getFloat(attachmentMap, "rotation", 0);
						regionAttachment->width = Json_getFloat(attachmentMap, "width", 32) * self->scale;
						regionAttachment->height = Json_getFloat(attachmentMap, "height", 32) * self->scale;
						RegionAttachment_updateOffset(regionAttachment);
					}

					Skin_addAttachment(skin, slotIndex, skinAttachmentName, attachment);
				}
			}
		}
	}

	animations = Json_getItem(root, "animations");
	if (animations) {
		int animationCount = Json_getSize(animations);
		skeletonData->animations = MALLOC(Animation*, animationCount);
		for (i = 0; i < animationCount; ++i) {
			Json* animationMap = Json_getItemAt(animations, i);
			_SkeletonJson_readAnimation(self, animationMap, skeletonData);
		}
	}

	Json_dispose(root);
	return skeletonData;
}
