#include <cppunit/TextTestRunner.h>
#include <cppunit/extensions/TestFactoryRegistry.h>

#ifdef __APPLE_CC__
	#include <GLUT/glut.h>
#else
	#include <GL/glut.h>
#endif

int main(int argc, const char* argv[])
{
    CppUnit::TextTestRunner runner;
    runner.addTest(CppUnit::TestFactoryRegistry::getRegistry().makeTest());

    runner.run();

    return 0;
}


void buildGLContext()
{
	static bool glutInitialised = false;
    char* pointer;
    int number;
    
    if( !glutInitialised)
    {
        glutInit( &number, &pointer);
        glutInitDisplayMode(GLUT_DEPTH | GLUT_RGB | GLUT_DOUBLE | GLUT_MULTISAMPLE);
        glutInitWindowPosition(0, 0);
        glutInitWindowSize( 150, 150);
        glutCreateWindow("FTGL TEST");
        
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D( 0.0, 150, 0.0, 150);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        
        glutInitialised = true;
    }
}

