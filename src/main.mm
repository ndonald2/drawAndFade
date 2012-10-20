#import <Cocoa/Cocoa.h>

int main(int argc, char *argv[])
{
    [NSApplication sharedApplication];
    NSString *appNib = [[NSBundle mainBundle] objectForInfoDictionaryKey:@"NSMainNibFile"];
    [NSBundle loadNibNamed:appNib owner:NSApp];
    [NSApp run];
    
    return 0;
}
//--------------------------------------------------------------
//int main(){
//	ofAppGlutWindow window; // create a window
//	// set width, height, mode (OF_WINDOW or OF_FULLSCREEN)
//	ofSetupOpenGL(&window, 1024, 768, OF_WINDOW);
//	ofRunApp(new ofApplication()); // start the app
//}
