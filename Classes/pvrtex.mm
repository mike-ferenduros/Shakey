#include "pvrtex.h"
#include <OpenGLES/ES1/gl.h>
#include <OpenGLES/ES1/glext.h>
#include <CoreFoundation/CoreFoundation.h>



typedef unsigned int u32;


typedef struct _PVRTexHeader
{
	u32 headerLength;
	u32 height;
	u32 width;
	u32 numMipmaps;
	u32 flags;
	u32 dataLength;
	u32 bpp;
	u32 bitmaskRed;
	u32 bitmaskGreen;
	u32 bitmaskBlue;
	u32 bitmaskAlpha;
	u32 pvrTag;
	u32 numSurfs;
}
PVRTexHeader;

static int mipsize_for_dims( int width, int height, int flags )
{
	int bpp, blocksize;
	if( (flags & 0xFF) == 24 )
	{
		blocksize = 8 * 4; // Pixel by pixel block size for 2bpp
		width = width / 8;
		height = height / 4;
		bpp = 2;
	}
	else
	{
		blocksize = 4 * 4; // Pixel by pixel block size for 4bpp
		width = width / 4;
		height = height / 4;
		bpp = 4;
	}

	// Clamp to minimum number of blocks
	if( width < 2 )		width = 2;
	if( height < 2 )	height = 2;
	
	return( width * height * ((blocksize*bpp)/8) );
}

static int load_pvrtex_from_data( const void *data_p )
{
	//Allocate a texture
	GLuint texid;
	glGenTextures( 1, &texid );
	if( texid == 0 )
		return( 0 );
	glBindTexture( GL_TEXTURE_2D, texid );
	
	const PVRTexHeader *hdr_p = (PVRTexHeader*)data_p;

	u32 hdr_flags = CFSwapInt32LittleToHost( hdr_p->flags );
	u32 hdr_width = CFSwapInt32LittleToHost( hdr_p->width );
	u32 hdr_height = CFSwapInt32LittleToHost( hdr_p->height );
	u32 hdr_dataLength = CFSwapInt32LittleToHost( hdr_p->dataLength );
	u32 hdr_numMipmaps = CFSwapInt32LittleToHost( hdr_p->numMipmaps );

	//Determine the right texture-format to use
	int ifmt;
	if( (hdr_flags & 0xFF) == 24 )
		ifmt = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
	else if( (hdr_flags & 0xFF) == 25 )
		ifmt = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;

	//Now just keep reading miplevels until we run out of file
	unsigned char *pixels_p = (unsigned char *)(hdr_p+1);
	unsigned char *end_p = pixels_p + hdr_dataLength;
	int miplevel = 0;
	int mipwidth = hdr_width;
	int mipheight = hdr_height;

	while( (pixels_p < end_p) && (miplevel < hdr_numMipmaps+1) )
	{
		int mipsize = mipsize_for_dims( mipwidth, mipheight, hdr_flags );

		glCompressedTexImage2D( GL_TEXTURE_2D, miplevel, ifmt, mipwidth, mipheight, 0, mipsize, pixels_p );

		mipwidth = mipwidth>1 ? (mipwidth>>1) : 1;
		mipheight = mipheight>1 ? (mipheight>>1) : 1;
		miplevel++;
		pixels_p += mipsize;
	}

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	NSLog( @"%i = %d miplevels, base %d x %d\n", texid, miplevel, hdr_width, hdr_height );

	return( texid );
}

int load_pvrtex( const char *basename_p )
{
	NSString *path = [[NSBundle mainBundle] pathForResource:[NSString stringWithCString:basename_p encoding:NSUTF8StringEncoding] ofType:@"pvr"];

	NSData *data_p = [NSData dataWithContentsOfFile:path];

	return( load_pvrtex_from_data([data_p bytes]) );
}
