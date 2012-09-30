
class CScreenshotSurface {

public:
  int            width;
  int            height;
  int            stride;
  unsigned char* buffer;

  CScreenshotSurface(void);
  boolean capture( void );
};

class CScreenShot {

public:
  static void TakeScreenshot();
  static void TakeScreenshot(const CStdString &filename, bool sync);
};