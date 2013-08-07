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

#import "InspectorSkeletonAnimations.h"
#import "CocosBuilderAppDelegate.h"
#import "SkeletonPropertySetter.h"

@implementation InspectorSkeletonAnimations

@synthesize animations=_animations;

- (void)setAnimations:(NSMutableArray *)animations
{
    if (!_animations)
        _animations = [[NSMutableArray alloc] init];
}

- (NSMutableArray *)animations
{
    if (!_animations)
        _animations = [[NSMutableArray alloc] init];
    
    CCSkeletonAnimation *skeleton = (CCSkeletonAnimation *)selection;
    
    if (skeleton.skeleton)
    {
        Animation **animations = skeleton.skeleton->data->animations;
    
        for (int i = 0; i < skeleton.skeleton->data->animationCount; i++) {
            NSString *animationName = [NSString stringWithUTF8String:(*(animations+i))->name];
            NSDictionary *dictionary = @{@"animation" : animationName};
            [_animations addObject:dictionary];
        }
        
        NSDictionary *dictionary = @{@"animation" : @"stop"};
        [_animations addObject:dictionary];
    }

    return _animations;
}

- (BOOL)tableView:(NSTableView *)tableView shouldSelectRow:(NSInteger)row
{
    NSDictionary *animation = [_animations objectAtIndex:row];
    NSString *animationName = [animation valueForKey:@"animation"];
    
    CCSkeletonAnimation *skeleton = (CCSkeletonAnimation *)selection;
    
    if ([animationName isEqualToString:@"pause"]) {
        [skeleton clearAnimation];
    } else
        [skeleton setAnimation:animationName loop:YES];
    
    return YES;
}

@end
