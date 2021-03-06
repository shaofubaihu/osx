/* APPLE LOCAL file radar 4805321 */
/* Test sequence of assignment to setters. */
/* { dg-options "-mmacosx-version-min=10.5 -fobjc-new-property -lobjc" { target powerpc*-*-darwin* i?86*-*-darwin* } } */
/* { dg-options "-fobjc-new-property -lobjc" { target arm*-*-darwin* } } */
/* { dg-do run { target *-*-darwin* } } */

#include <objc/objc.h>
/* APPLE LOCAL radar 4894756 */
#include "../objc/execute/Object2.h"
extern "C" void abort (void);

@interface Bar : Object
{
  int iVar;
  int iBar;
  float f;
}
@property (assign, setter = MySetter:) int prop1;
@property (assign) int prop2;
@property (assign) float fprop;
@end

@implementation Bar
@synthesize prop1=iVar;
@synthesize prop2=iBar;
@synthesize fprop=f;

- (void) MySetter : (int) value { iVar = value; }

@end

int main(int argc, char *argv[]) {
    Bar *f = [Bar new];
    if (f.prop2 = 1)
      f.prop2 = f.prop1 = -4;
    if (f.prop1 == -4)
      f.prop2 = f.prop1 = 5;

    if (f.prop1 != 5 || f.prop2 != 5)
      abort ();

    f.fprop = 3.14;
    f.prop1 = f.prop2 = f.fprop;
    if (f.prop1 != 3 || f.prop2 != 3)
      abort ();
    while (f.prop1)
      f.prop1 -= 1;
    return f.prop1;
}

