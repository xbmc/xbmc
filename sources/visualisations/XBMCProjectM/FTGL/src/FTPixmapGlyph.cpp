#include    "FTPixmapGlyph.h"

FTPixmapGlyph::FTPixmapGlyph( FT_GlyphSlot glyph)
:   FTGlyph( glyph),
    destWidth(0),
    destHeight(0),
    data(0)
{
    err = FT_Render_Glyph( glyph, FT_RENDER_MODE_NORMAL);
    if( err || ft_glyph_format_bitmap != glyph->format)
    {
        return;
    }

    FT_Bitmap bitmap = glyph->bitmap;

    //check the pixel mode
    //ft_pixel_mode_grays
        
    int srcWidth = bitmap.width;
    int srcHeight = bitmap.rows;
    
    destWidth = srcWidth;
    destHeight = srcHeight;
    
    if( destWidth && destHeight)
    {
        data = new unsigned char[destWidth * destHeight * 2];
        unsigned char* src = bitmap.buffer;

        unsigned char* dest = data + ((destHeight - 1) * destWidth * 2);
        size_t destStep = destWidth * 2 * 2;

        for( int y = 0; y < srcHeight; ++y)
        {
            for( int x = 0; x < srcWidth; ++x)
            {
                *dest++ = static_cast<unsigned char>(255);
                *dest++ = *src++;
            }
            dest -= destStep;
        }

        destHeight = srcHeight;
    }

    pos.X(glyph->bitmap_left);
    pos.Y(srcHeight - glyph->bitmap_top);
}


FTPixmapGlyph::~FTPixmapGlyph()
{
    delete [] data;
}


const FTPoint& FTPixmapGlyph::Render( const FTPoint& pen)
{
    glBitmap( 0, 0, 0.0f, 0.0f, pen.X() + pos.X(), pen.Y() - pos.Y(), (const GLubyte*)0);
    
    if( data)
    {
        glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);
        glPixelStorei( GL_UNPACK_ALIGNMENT, 2);

        glDrawPixels( destWidth, destHeight, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, (const GLvoid*)data);
    }
        
    glBitmap( 0, 0, 0.0f, 0.0f, -pos.X(), pos.Y(), (const GLubyte*)0);

    return advance;
}
