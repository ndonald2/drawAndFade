#include "ofAppGlutWindow.h"
#include "ofApplication.h"
#import <Cocoa/Cocoa.h>

int main(int argc, char *argv[])
{
    NSString *appNib = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSMainNibFile"];
    [NSBundle loadNibNamed:appNib owner:[NSApplication sharedApplication]];
    [[NSApplication sharedApplication] run];
    return 0;
}
//--------------------------------------------------------------

//#include "ofApplication.h"
//#include "ofAppGlutWindow.h"
//#include <Quartz/Quartz.h>
//
//int main(){
//	ofAppGlutWindow window; // create a window
//	// set width, height, mode (OF_WINDOW or OF_FULLSCREEN)
//	ofSetupOpenGL(&window, 1920, 1200, OF_FULLSCREEN);
//	ofRunApp(new ofApplication()); // start the app
//}
