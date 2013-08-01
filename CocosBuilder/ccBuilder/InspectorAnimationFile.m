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

#import "InspectorAnimationFile.h"
#import "ResourceManager.h"
#import "ResourceManagerUtil.h"
#import "CCBGlobals.h"
#import "CocosBuilderAppDelegate.h"
#import "CCBDocument.h"
#import "CCBReaderInternal.h"
#import "SkeletonPropertySetter.h"
#import "PositionPropertySetter.h"
#import "CCNode+NodeInfo.h"

@implementation InspectorAnimationFile

- (void) willBeAdded
{
    // Setup menu
    NSString* sf = [selection extraPropForKey:propertyName];
    
    [ResourceManagerUtil populateResourcePopup:popup resType:kCCBResTypeJSON allowSpriteFrames:NO selectedFile:sf selectedSheet:NULL target:self];
}

- (void) selectedResource:(id)sender
{
    [[CocosBuilderAppDelegate appDelegate] saveUndoStateWillChangeProperty:propertyName];
    
    id item = [sender representedObject];
    
    NSString* AnimationFile = NULL;
    
    if ([item isKindOfClass:[RMResource class]])
    {
        RMResource* res = item;
        
        if (res.type == kCCBResTypeJSON)
        {
            AnimationFile = [ResourceManagerUtil relativePathFromAbsolutePath:res.filePath];
            [ResourceManagerUtil setTitle:AnimationFile forPopup:popup];
        }
    }
    
    if (AnimationFile)
    {
        [selection setExtraProp:AnimationFile forKey:propertyName];
        [SkeletonPropertySetter setAnimationFileForNode:(CCSkeletonAnimation *)selection andProperty:propertyName withFile:AnimationFile];
    }
    
    [self updateAffectedProperties];
    
    // Reload the inspector
    [[CocosBuilderAppDelegate appDelegate] performSelectorOnMainThread:@selector(updateInspectorFromSelection) withObject:NULL waitUntilDone:NO];
}

@end
