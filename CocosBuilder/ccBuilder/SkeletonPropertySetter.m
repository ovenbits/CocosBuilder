/*
 * CocosBuilder: http://www.cocosbuilder.com
 *
 * Copyright (c) 2012 Zynga Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#import "SkeletonPropertySetter.h"
#import "CocosBuilderAppDelegate.h"
#import "CCBGlobals.h"
#import "CCBWriterInternal.h"
#import "ResourceManager.h"
#import "CCBFileUtil.h"
#import "CCNode+NodeInfo.h"
#import "SequencerHandler.h"
#import "CCBReaderInternal.h"

@implementation SkeletonPropertySetter

+ (void)setJSONFileForNode:(CCSkeletonAnimation *)node andProperty:(NSString *)prop withFile:(NSString *)jsonFile
{
    NSString *jsonFileName = [[ResourceManager sharedManager] toAbsolutePath:jsonFile];
    bool newJSONFile = ([node.jsonFile isEqualToString:jsonFileName]) ? false : true;
    node.jsonFile = jsonFileName;
    [node setValue:jsonFileName forKey:prop];    
    
    if (!node.atlasFile) return;

    NSString *atlasFileName = node.atlasFile;

    if (!node.skeleton || newJSONFile)
        [SkeletonPropertySetter createSkeletonForNode:node JSONFile:jsonFileName atlasFile:atlasFileName];
}

+ (void)setAtlasFileForNode:(CCSkeletonAnimation *)node andProperty:(NSString *)prop withFile:(NSString *)atlasFile
{
    NSLog(@"ATLAS FILE: %@",atlasFile);
    NSString *atlasFileName = [[ResourceManager sharedManager] toAbsolutePath:atlasFile];
    bool newAtlasFile = ([node.atlasFile isEqualToString:atlasFileName]) ? false : true;
    node.atlasFile = atlasFileName;
    [node setValue:atlasFileName forKey:prop];
    
    if (!node.jsonFile) return;
    
    NSString *jsonFileName = node.jsonFile;
    
    if (!node.skeleton || newAtlasFile)
        [SkeletonPropertySetter createSkeletonForNode:node JSONFile:jsonFileName atlasFile:atlasFileName];
}

+ (void)createSkeletonForNode:(CCSkeletonAnimation*)node JSONFile:(NSString*)jsonFileName atlasFile:(NSString*)atlasFileName
{
    NSFileManager *fileManager = [NSFileManager defaultManager];
    if (jsonFileName && atlasFileName) {
        
        if ([fileManager fileExistsAtPath:jsonFileName] && [fileManager fileExistsAtPath:atlasFileName]) {
            
            NSString *skinName = [atlasFileName lastPathComponent];
            
            Atlas *atlas = Atlas_readAtlasFile([atlasFileName UTF8String]);
            [node setAtlas:atlas];
            
            SkeletonJson* json = SkeletonJson_create(atlas);
            json->scale = 1.0;
            SkeletonData* skeletonData = SkeletonJson_readSkeletonDataFile(json, [jsonFileName UTF8String]);
            SkeletonJson_dispose(json);
            
            if (skeletonData) {
                [node initialize:skeletonData ownsSkeletonData:YES];
                [node setSkin:[skinName stringByDeletingPathExtension]];
                [node initialize];
                [node setToSetupPose];
                
                node.timeScale = 1.0f;
            }
        }
    }
}

+ (void)setAnimationFileForNode:(CCSkeletonAnimation *)node andProperty:(NSString *)prop withFile:(NSString *)animationFile
{
    [node setValue:animationFile forKey:prop];
}

+ (void)setControllerFileForNode:(CCSkeletonAnimation *)node andProperty:(NSString *)prop withFile:(NSString *)controllerFile
{
    [node setValue:controllerFile forKey:prop];
}

+ (void)setSkeleton:(CCSkeletonAnimation *)node debugBones:(BOOL)debugBones
{
    node.debugBones = debugBones;
}

+ (void)setSkeleton:(CCSkeletonAnimation *)node debugSlots:(BOOL)debugSlots
{
    node.debugSlots = debugSlots;
}

@end
