#include <jni.h>

#include <android/log.h>
#include <android/bitmap.h>

#include "opencv2/imgproc.hpp"

#define  LOG_TAG    "Adaptive"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)



#ifdef __cplusplus
extern "C" {
#endif


cv::Mat mAdaptive;


JNIEXPORT void JNICALL
Java_ph_edu_dlsu_adaptive_CameraActivity_process(JNIEnv *env, jobject instance,
                                                            jobject pTarget, jbyteArray pSource, jint threshold) {
    double_t start;
    
    jint blockSize= 15; // size of the neighborhood
    jint halfSize= blockSize/2;

    AndroidBitmapInfo bitmapInfo;
    uint32_t* bitmapContent;

    size_t roi_side;

    if(AndroidBitmap_getInfo(env, pTarget, &bitmapInfo) < 0) abort();
    if(bitmapInfo.format != ANDROID_BITMAP_FORMAT_RGBA_8888) abort();
    if(AndroidBitmap_lockPixels(env, pTarget, (void**)&bitmapContent) < 0) abort();

    /// Access source array data... OK
    jbyte* source = (jbyte*)env->GetPrimitiveArrayCritical(pSource, 0);
    if (source == NULL) abort();

    /// cv::Mat for YUV420sp source and output BGRA
    cv::Mat srcGray(bitmapInfo.height, bitmapInfo.width, CV_8UC1, (unsigned char *)source);
    cv::Mat mbgra(bitmapInfo.height, bitmapInfo.width, CV_8UC4, (unsigned char *)bitmapContent);


/***********************************************************************************************/
    /// Native Image Processing HERE...

    start = static_cast<double_t>(cv::getTickCount());

    /// Take care of front camera horizontal flipping
    cv::flip(srcGray, srcGray, 1); // 1 = horizontal flipping

    roi_side = std::min(bitmapInfo.height, bitmapInfo.width);

    /// Region of interest in the center
    if (mAdaptive.empty())
        mAdaptive = srcGray(cv::Range::all(), cv::Range((bitmapInfo.width - roi_side)/2, (bitmapInfo.width - roi_side)/2 + roi_side));
        
    /// Compute integral image
	cv::Mat integralImg;
	cv::integral(mAdaptive, integralImg, CV_32S);


    /// Apply adaptive threshold
    for (int j = halfSize; j < mAdaptive.rows - halfSize-1; j++) { // blocksize is a regular n x n block

		  // get the address of row j
		  uchar* data= mAdaptive.ptr<uchar>(j);
		  int* idata1= integralImg.ptr<int>(j-halfSize);
		  int* idata2= integralImg.ptr<int>(j+halfSize+1);

          for (int i = halfSize; i < mAdaptive.cols - halfSize - 1; i++) {
 
			  // compute mean
			  int mean = (idata2[i+halfSize+1] - idata2[i-halfSize] -
				        idata1[i+halfSize+1] + idata1[i-halfSize])/(blockSize * blockSize);

			  // apply adaptive threshold
			  if (data[i] < (mean - threshold))
				  data[i] = 0;
				  
			  else

				  data[i] = 255;
         }                    
    }
    

    LOGI("Processing took %0.2lf ms.", 1000.0 * (static_cast<double>(cv::getTickCount()) - start) / cv::getTickFrequency());

    cvtColor(srcGray, mbgra, CV_GRAY2BGRA);

/************************************************************************************************/

    /// Release Java byte buffer and unlock backing bitmap
    //env-> ReleasePrimitiveArrayCritical(pSource,source,0);
    /*
     * If 0, then JNI should copy the modified array back into the initial Java
     * array and tell JNI to release its temporary memory buffer.
     *
     * */

    env-> ReleasePrimitiveArrayCritical(pSource, source, JNI_COMMIT);
    /*
 * If JNI_COMMIT, then JNI should copy the modified array back into the
 * initial array but without releasing the memory. That way, the client code
 * can transmit the result back to Java while still pursuing its work on the
 * memory buffer
 *
 * */

    /*
     * Get<Primitive>ArrayCritical() and Release<Primitive>ArrayCritical()
     * are similar to Get<Primitive>ArrayElements() and Release<Primitive>ArrayElements()
     * but are only available to provide a direct access to the target array
     * (instead of a copy). In exchange, the caller must not perform blocking
     * or JNI calls and should not hold the array for a long time
     *
     */

    if (AndroidBitmap_unlockPixels(env, pTarget) < 0) abort();
}


#ifdef __cplusplus
}
#endif
