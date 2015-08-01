/**
@file v4l2_camera.cpp
*/
#ifndef _V4L2_CAMERA_H_
#define _V4L2_CAMERA_H_

/*! \mainpage
 * <b>This is a C++ wrapper for well known library Video4Linux2.</b>\n
 * It allows to get image from camera easily as shown in the following example:\n
 * \code    {.cpp}
 * V4L2::Camera camera(640, 480);
 * // Select device
 * camera.setDevice("/dev/video0");
 * // Open device
 * camera.open();
 * // Start capturing
 * camera.startCapturing();
 * // While capturing it is possible to get image
 * unsigned char *image = camera.getImage();
 * // Release resources before exit
 * camera.stopCapturing();
 * camera.close();
 * \endcode
 */

#include <fcntl.h>
#include <sys/mman.h>
#include <linux/videodev2.h>
#include "/usr/include/libv4l2.h"
#include <thread>
#include <mutex>

/**
 * The namespace of the wrapper.
 */
namespace V4L2 {

    /**
     * Errors enumeration
     */
    enum {
        CAMERA_SUCCESS = 0,///<Function succeeded.
        CAMERA_BAD_STATE,///<Bad order of the current call. The correct order is described in V4L2::Camera reference.
        CAMERA_CANNOT_OPEN,///<Can't open device. Probably it doesn't exist or it is busy now.
        CAMERA_WRONG_PIXELFORMAT,///<Set pixelformat wasn't accepted.
        CAMERA_ERROR,///<Unknown error.
        CAMERA_DIFFERENT_SIZE///<Changed size to not the given one. Get current size using getSize().
    };

    /**
     * Return values of the callback
     */
    typedef enum {
        CONTINUE,///<Continue getting images
        STOP///<Stop getting images
    } ContinousControl;

    /**
     * This class maganes camera using V4L2 API. Using this class you can easily get Image and manipulate parameters.
     */
    class Camera 
    {
        public:
            /**
             * Callback (it may be lambda expression too. to call when using synchronous option.
             * @param bytes table of char in format specified in v4l2 documentation.
             * \code    {.cpp}
             * //Example of lambda
             * [&](char *imag){
             *     //here receive image
             *     return CAMERA_ASYNC_STOP;//stop receiving
             * }
             * \endcode
             * @return CAMERA_ASYNC_CONTINUE to continue getting image
             * @return CAMERA_ASYNC_STOP to finish getting image
             * @return numer > 0 to get image for nubers of second

             */
            typedef ContinousControl (*sync_callback)(unsigned char* bytes);

            /**
             * Constructor without parameters. It means that before using camera, you have to set the parameters manually or it will use default size 640x480.
             * @see setSettings()
             * @see setDevice()
             */
            Camera();

            /**
             * Constructor with parameters describing camera size.
             * @param width width in pixels.
             * @param height height in pixels.
             * @param pix_format format of the output image.
             * @see setSettings()
             * @see setDevice()
             */
            Camera(int width, int height, int pix_format = V4L2_PIX_FMT_RGB24);

            /**
             * Destructor of Camera class. It closes device and frees memory automatic.
             */
            ~Camera();

            /**
             * Gets image once. If there is latency in getting current frame, use reopen() before.
             * @see reopen()
             * \pre open() has to be called
             * \pre startCapturing() has to be called
             * @return pointer to buffer with allocated image data in set V4L2 format on success
             * \post you mustn't free returned pointer
             * @returns NULL on any issue
             */
            unsigned char* getImage();

            /**
             * Receive images synchronously, as long as callback returns CAMERA_ASYNC_CONTINUE. It calls callback, given as parameter.
             * When camera is not opened, it calls open()
             * @param callback calback or lambda expression to call
             * @return CAMERA_BAD_STATE
             * @return CAMERA_SUCCESS
             * \pre open() has to be called
             * \pre startCapturing() has to be called
             * @see sync_callback()
             * @see open()
             */
            int getImagesContinuously(sync_callback callback);


            /**
             * Opens camera for capturing. Before retriving images, you need to call startCapturing()
             * After opening you can get current image size using getSize
             * @see startCapturing()
             * @see getSize()
             * @return 0 success
             * @return CAMERA_BAD_STATE bad state
             * @return CAMERA_CANNOT_OPEN device not availble
             * @return CAMERA_WRONG_PIXELFORMAT Libv4l didn't accept color format
             */
            int open();

            /**
             * Starts capturing.
             * All settings has to be set before startCapturing() will be called
             * \pre open() has to be called
             * @see open()
             */
            int startCapturing();

            /**
             * Stops capturing. May be used when camera is not used for a moment or we want to change its parameters.
             * @return CAMERA_BAD_STATE
             * @return CAMERA_SUCCESS
             */
            int stopCapturing();

            /**
             * Closes device. After that you can change parameters and it may be opened once again using open().
             * If you want to reset camera, use reopen() instead
             * @see open()
             * @see reopen()
             *
             * @return CAMERA_BAD_STATE
             * @return CAMERA_SUCCESS
             */
            int close();

            /**
             * Stops and starts capturing. It may be usefull when we need reset camera
             * @see open()
             * @see close()
             */
            int restartCapturing();

            /**
             * Closes and Opens camera. It may be usefull when we need reset camera
             * @see open()
             * @see close()
             */
            int reopen();

            /**
             * Sets dimensions. When capturing is in process it changes parameters however resolution will be changed after reopen()
             * @param width width of image
             * @param height height of image
             * @return CAMERA_SUCCESS
             * @return CAMERA_BAD_STATE when called after startCapturing() or before open()
             * @see reopen()
             */
            int setSize(int width, int height);

            /**
             * Gets dimensions.
             * If you set dimension, which wasn't accept by device, it had set similar dimension. To get it, use this function
             * @param width pointer for width
             * @param height pointer for height
             * @see reopen()
             */
            void getSize(int *width, int *height);


            /**
             * Sets device. By default it is /dev/video0
             * If device doesn't exists, there will be error while open is call
             * To let device change, you have to call open()
             * @see open()
             */
            void setDevice(std::string const& device);

            /**
             * Changes settings using v4l2 structures.
             * Availble structures:
             * @param request integer instruction (i.e. VIDEOC_S_CTRL etc.)
             * @param structure a v4l2 structure (i.e. v4l2_format, v4l2_buffer, v4l2_requestbuffers)
             * \code    {.cpp}
             * V4L2::Camera camera;
             * ...
             * struct v4l2_format fmt;
             * fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
             * fmt.fmt.pix.width       = 160;
             * fmt.fmt.pix.height      = 140;
             * fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
             * fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;
             * camera.setSettings(VIDIOC_S_FMT, &fmt);
             * if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24) {
             *      printf("Libv4l didn't accept RGB24 format. Can't proceed.\n");
             *      return CAMERA_WRONG_PIXELFORMAT;
             * }
             * \endcode
             *
             * \code    {.cpp}
             * // Disable auto focus
             * struct v4l2_control control;
             * control.id = V4L2_CID_FOCUS_AUTO;
             * control.value = false;
             * ret = camera.setSettings(VIDIOC_S_CTRL, &control);
             * if (ret != CAMERA_SUCCESS) // Handle error
             * \endcode
             *
             * @pre open()
             */
            int setSettings(int request, void* structure);

        private:
            enum camera_state {
                CLOSED,
                STARTED,
                STOPPED,
                CONTINOUS
            } state;

            std::pair<int,int> camera_size;

            // Variables needed by V4L2
            struct v4l2_format              fmt;
            struct v4l2_buffer              buf;
            struct v4l2_requestbuffers      req;
            enum v4l2_buf_type              type;
            struct timeval                  tv;
            int                             fd = -1;
            int                             pix_fmt;
            unsigned int                    n_buffers;
            std::string                     dev_name;
            bool stop_flag;

            struct buffer {
                void   *start;
                size_t length;
            } *buffers;

            /**
             * Initialize and deinitialize buffer using mmap
             */
            int prepare();
            int unprepare();

            /**
             * Function calling v4l2 requests
             * @return 1 on error
             * @return 0 on success
             */
            int xioctl(int fh, int request, void *arg);

            std::mutex mutex;
    };
}

#endif // _V4L2_CAMERA_H_
