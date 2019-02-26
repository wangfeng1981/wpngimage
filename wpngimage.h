
#include "png.h"
#include <vector>
#include <string>

using std::string;
using std::vector;

#ifndef W_PNG_IMAGE

class wPngImage
{
	unsigned char* pImageData ;
	int imageDataSize ;

	int width;
	int height ;
	inline wPngImage(wPngImage& right){};
public:
	bool openFromFile32( string fname ) ;
	bool openFromMem32( unsigned char* buff , size_t buffsize ) ;
	bool writeToFile32( string fname ) ;
	bool writeToMem32() ;
	bool create(int wid,int hei) ;

	inline wPngImage(){pImageData=0;pFileBinary=0;imageDataSize=0;fileBinarySize=0;};
	inline ~wPngImage(){destroy();};
	inline void destroy(){if(pImageData) delete[]pImageData;pImageData=0;if(pFileBinary) delete[]pFileBinary;pFileBinary=0;imageDataSize=0;fileBinarySize=0;fileBinaryStorage=0;} ;
	inline unsigned char* getImageData(){return pImageData;};
	inline unsigned char* getFileBinary(){return pFileBinary;};
	inline int getImageDataSize(){return imageDataSize;};
	inline int getFileBinarySize(){return fileBinarySize;};

	int fileBinarySize ;
	int fileBinaryStorage ;
	unsigned char* pFileBinary ;
} ;

bool wPngImage::openFromFile32( string fname ) 
{
	this->destroy() ; 
    char header[8];
    FILE *fp = fopen(fname.c_str(), "rb");
    if (!fp) return false ;
    fread(header, 1, 8, fp);
    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) return false ;
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) return false;
    if (setjmp(png_jmpbuf(png_ptr))) return false;
    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, 8);
    png_read_info(png_ptr, info_ptr);
    this->width = png_get_image_width(png_ptr, info_ptr);
    this->height = png_get_image_height(png_ptr, info_ptr);
    png_byte color_type = png_get_color_type(png_ptr, info_ptr);//should be RGBA
    png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);//should be 8
    int number_of_passes = png_set_interlace_handling(png_ptr);
    png_read_update_info(png_ptr, info_ptr);
    if (setjmp(png_jmpbuf(png_ptr))) return false;
	this->imageDataSize = width * height *4 ;
	this->pImageData = new unsigned char[imageDataSize] ;
    for (int y=0; y<this->height; y++)
	{
		unsigned char* rowpointer = &(pImageData[y*this->width*4]) ;
		png_read_row(png_ptr, rowpointer , 0);
	}
    fclose(fp);
	return true ;
}

struct wPngMemoryBlock{
	unsigned char* pbuff ;
	int location ;
	int buffsize ;
	bool idestroy ;
	inline wPngMemoryBlock(){pbuff=0;location=-1;buffsize=0;idestroy=false;} ;
	inline ~wPngMemoryBlock(){if(idestroy)delete[]pbuff;pbuff=0;} ;
	wPngMemoryBlock( string filepath ) ;
};

wPngMemoryBlock::wPngMemoryBlock( string filepath )
{
	pbuff = 0 ;
	FILE* pf = fopen( filepath.c_str() , "rb" ) ;
	fseek(pf, 0, SEEK_END);
	size_t fsize = ftell(pf);
	fseek(pf, 0, SEEK_SET); 
	pbuff = new unsigned char[fsize] ;
	fread(pbuff, fsize, 1, pf);
	fclose(pf) ;
	buffsize = fsize ;
	location = 0 ;
	idestroy = true ;
}

void wPngImageReadDataFromInputMemory(
	png_structp png_ptr, 
	png_bytep outBytes,
	png_size_t byteCountToRead)
{
   png_voidp io_ptr = png_get_io_ptr(png_ptr);
   if(io_ptr == 0)
      return;   // add custom error handling here
   wPngMemoryBlock* pmem = (wPngMemoryBlock*)io_ptr;
   int newreadlength = pmem->location + byteCountToRead ;
   if( newreadlength > pmem->buffsize )
   {
	   printf("error : exceed memblock size.\n") ;
   }else
   {
	   unsigned char* ptr1 = pmem->pbuff + pmem->location ;
	   memcpy( outBytes , ptr1 , byteCountToRead ) ;
	   pmem->location = newreadlength ;
   }
}  


bool wPngImage::openFromMem32( unsigned char* buff , size_t buffsize ) 
{
	this->destroy() ; 
    char header[8];
	memcpy( header , buff , 8 ) ;

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) return false ;
    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) return false;

	wPngMemoryBlock memblock ;
	memblock.pbuff = buff ;
	memblock.buffsize = buffsize ;
	memblock.location = 8 ;

	png_set_read_fn(png_ptr, &memblock  , wPngImageReadDataFromInputMemory);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);
	png_uint_32 width1 = 0;
	png_uint_32 height1 = 0;
	int bitDepth = 0;//should be 8
	int colorType = -1;//should be rgba
	png_uint_32 retval = png_get_IHDR(png_ptr, info_ptr,
	   &width1,
	   &height1,
	   &bitDepth,
	   &colorType,
	   NULL, NULL, NULL);
	this->width = width1 ;
	this->height = height1 ;
	if(retval != 1)
	   return false;	// add error handling and cleanup

	this->pImageData = new unsigned char[width*height*4] ;
	// read single row at a time
	for(int rowIdx = 0; rowIdx < height; ++rowIdx)
	{
		unsigned char* rowptr = this->pImageData + rowIdx * width *4 ;
		png_read_row(png_ptr, rowptr, NULL);
	}
	png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
	return true ;
}
bool wPngImage::writeToFile32( string fname ) 
{
	if( pImageData == 0 ) return false ;
    FILE *fp = fopen(fname.c_str(), "wb");
    if (!fp) return false;
    png_structp png_ptr1 = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr1)  return false;
    png_infop info_ptr1 = png_create_info_struct(png_ptr1);
    if (!info_ptr1) return false;
    if (setjmp(png_jmpbuf(png_ptr1))) return false;
    png_init_io(png_ptr1, fp);
	if (setjmp(png_jmpbuf(png_ptr1))) return false;
	png_set_compression_level(png_ptr1,0) ;//no compression.
	png_set_filter(png_ptr1,0,PNG_FILTER_NONE) ;
    png_set_IHDR(png_ptr1, info_ptr1, width, height,
                    8,//bit_depth
					PNG_COLOR_TYPE_RGB_ALPHA,
					PNG_INTERLACE_NONE,
                    PNG_COMPRESSION_TYPE_BASE,
					PNG_FILTER_TYPE_BASE);//PNG_FILTER_NONE  PNG_FILTER_TYPE_BASE
	png_write_info(png_ptr1, info_ptr1);
    if (setjmp(png_jmpbuf(png_ptr1))) return false;
	for( int irow = 0 ; irow < height ; ++ irow )
	{
		png_write_row(png_ptr1, &(pImageData[irow*width*4]) );
	}
    if (setjmp(png_jmpbuf(png_ptr1))) return false;
    png_write_end(png_ptr1, NULL);
    fclose(fp);
	return true ;
}

static void wPngImageMemoryWriter(png_structp  png_ptr, png_bytep data, png_size_t length) {
    wPngImage *wpngPtr = (wPngImage*)png_get_io_ptr(png_ptr);
	int newsize = wpngPtr->fileBinarySize + length ;
	if( newsize > wpngPtr->fileBinaryStorage )
	{
		unsigned char* newPtr = new unsigned char[newsize] ;
		memcpy( newPtr , wpngPtr->pFileBinary , newsize ) ;
		delete[] wpngPtr->pFileBinary ;
		wpngPtr->pFileBinary = newPtr ;
		wpngPtr->fileBinaryStorage = newsize ;
	}
	//append
	unsigned char* startPtr = &( wpngPtr->pFileBinary[wpngPtr->fileBinarySize] ) ;
	memcpy( startPtr , data , length ) ;
	wpngPtr->fileBinarySize = newsize ;
}

bool wPngImage::writeToMem32() 
{
	if( this->pImageData==0 ) return false ;
	if( this->pFileBinary )
	{
		delete [] this->pFileBinary ;
		this->fileBinarySize = 0 ;
		this->fileBinaryStorage = 0 ;
	}
	this->fileBinaryStorage = 260*1024 ;
	this->fileBinarySize = 0 ;
	this->pFileBinary = new unsigned char[this->fileBinaryStorage] ;
	png_structp p = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    png_infop info_ptr = png_create_info_struct(p);
	png_set_compression_level(p,0) ;//no compression.
	png_set_filter(p,0,PNG_FILTER_NONE) ;
    png_set_IHDR(p, info_ptr, width, height, 
		8,
		PNG_COLOR_TYPE_RGB_ALPHA,
		PNG_INTERLACE_NONE,
        PNG_COMPRESSION_TYPE_BASE,
		PNG_FILTER_TYPE_BASE);
    std::vector<unsigned char*> rows(height);
    for (size_t y = 0; y < height; ++y)
		rows[y] = (unsigned char*)(this->pImageData + y * width * 4);
    png_set_rows(p, info_ptr, &rows[0]);
    png_set_write_fn(p, this , wPngImageMemoryWriter, NULL);
    png_write_png(p, info_ptr, PNG_TRANSFORM_IDENTITY, NULL);
	png_destroy_write_struct(&p, &info_ptr);
	return true ;
}
bool wPngImage::create(int wid,int hei) 
{
	this->destroy() ;
	width = wid ;
	height = hei ;
	this->pImageData = new unsigned char[width*height*4] ;
	this->imageDataSize = width*height*4 ;
	memset(pImageData , 0 , this->imageDataSize ) ;
	return true ;
}









#endif 