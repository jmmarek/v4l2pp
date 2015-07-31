#include "v4l2_camera.h"

#define CLEAR(x) std::fill(x, sizeof(x))

using namespace V4L2;

Camera::Camera(){
    state = CLOSED;
    camera_size = std::pair<int, int>(640, 480);
    pix_fmt = V4L2_PIX_FMT_RGB24;
}

Camera::Camera(int width, int height, int pix_format){
    state = CLOSED;
    camera_size = std::pair<int, int>(width, height);
    this->pix_fmt = pix_format;
}

void Camera::setDevice(std::string device){
    dev_name = device;
}

void Camera::getSize(int *width, int *height)
{
    *width = camera_size.first;
    *height = camera_size.second;
}

int Camera::setSize(int width, int height)
{
    if(state != STOPPED)
        return CAMERA_BAD_STATE;

//    CLEAR(fmt);
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    fmt.fmt.pix.width       = width;
    fmt.fmt.pix.height      = height;
    fmt.fmt.pix.pixelformat = pix_fmt;
    fmt.fmt.pix.field       = V4L2_FIELD_INTERLACED;

    int ret = setSettings(VIDIOC_S_FMT, &fmt);
    if(ret != 0)
        return CAMERA_ERROR;

    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB24)
        return CAMERA_WRONG_PIXELFORMAT;

    camera_size.first = fmt.fmt.pix.width;
    camera_size.second = fmt.fmt.pix.height;
    if(camera_size.first != width || camera_size.second != height)
        return CAMERA_DIFFERENT_SIZE;

    return CAMERA_SUCCESS;
}

int Camera::reopen(){
    close();
    return open();
}

int Camera::setSettings(int request, void* _v4l2_structure)
{
    return xioctl(fd, request, _v4l2_structure);
}

int Camera::open(){
    if(state != CLOSED)
        return CAMERA_BAD_STATE;

    fd = v4l2_open(dev_name.c_str(), O_RDWR | O_NONBLOCK, 0);
    if (fd < 0){
        v4l2_close(fd);
        state = CLOSED;

        return CAMERA_CANNOT_OPEN;
    }
    state = STOPPED;

    return setSize(std::get<0>(camera_size), std::get<1>(camera_size));
}

int Camera::unprepare(){
    int ret;
    for (unsigned int i = 0; i < n_buffers; ++i){
        ret = v4l2_munmap(buffers[i].start, buffers[i].length);
        if(ret == -1)
            return CAMERA_ERROR;
    }
    free(buffers);
    return CAMERA_SUCCESS;
}

int Camera::prepare(){
    //CLEAR(req);
    req.count = 4;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_REQBUFS, &req);

    buffers = (buffer*) calloc(req.count, sizeof(*buffers));
    for (n_buffers = 0; n_buffers < req.count; ++n_buffers) {
        struct v4l2_buffer buf;
        //CLEAR(buf);

        buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory      = V4L2_MEMORY_MMAP;
        buf.index       = n_buffers;

        xioctl(fd, VIDIOC_QUERYBUF, &buf);

        buffers[n_buffers].length = buf.length;
        buffers[n_buffers].start = v4l2_mmap(NULL, buf.length,
                PROT_READ | PROT_WRITE, MAP_SHARED,
                fd, buf.m.offset);

        if (MAP_FAILED == buffers[n_buffers].start) {
            perror("mmap");
            return CAMERA_ERROR;
        }
    }

    return CAMERA_SUCCESS;
}

int Camera::startCapturing()
{
    if(state != STOPPED)
        return CAMERA_BAD_STATE;
    if(prepare() != CAMERA_SUCCESS)
        return CAMERA_ERROR;

    for (unsigned int i = 0; i < n_buffers; ++i) {
        struct v4l2_buffer buf;
       // CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        xioctl(fd, VIDIOC_QBUF, &buf);
    }

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = xioctl(fd, VIDIOC_STREAMON, &type);
    if(ret == -1)
        return CAMERA_ERROR;
    state = STARTED;


    return CAMERA_SUCCESS;
}

Camera::~Camera(){
    stopCapturing();
    close();
}

int Camera::getImagesSynchronously(sync_callback callback)
{
    mutex.lock();
    if(state != STARTED)
        return CAMERA_BAD_STATE;
    state = CONTINOUS;
    mutex.unlock();
    int run = CAMERA_ASYNC_CONTINUE;
    while (run != CAMERA_ASYNC_STOP) {
        //CLEAR(buf);
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        xioctl(fd, VIDIOC_DQBUF, &buf);

        if(stop_flag)
            break;
        run = callback((unsigned char*)buffers[buf.index].start);

        xioctl(fd, VIDIOC_QBUF, &buf);
    }
    mutex.lock();
    state = STARTED;
    mutex.unlock();

    return CAMERA_SUCCESS;
}

int Camera::restartCapturing()
{
    stopCapturing();
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    return startCapturing();
}

int Camera::stopCapturing()
{
    mutex.lock();
    int state_cp = state;
    mutex.unlock();

    if(state_cp == CONTINOUS){
        stop_flag = true;
        int i = 0;
        while(state == CONTINOUS || i == 3){
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            i++;
        }
        stop_flag = false;
        if(i == 4)
            return CAMERA_BAD_STATE;
    }
    if(state_cp != STARTED)
        return CAMERA_BAD_STATE;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    int ret = xioctl(fd, VIDIOC_STREAMOFF, &type);
    if(ret == -1)
        return CAMERA_ERROR;
    ret = unprepare();

    mutex.lock();
    state = STOPPED;
    mutex.unlock();

    return ret;
}

int Camera::close()
{
    if(state != STOPPED)
        return CAMERA_BAD_STATE;
    v4l2_close(fd);
    state = CLOSED;
    return CAMERA_SUCCESS;
}

unsigned char *Camera::getImage(){
    if(state != STARTED)
        return NULL;
    state = CONTINOUS;

    fd_set fds;
    int r = -1;
    do {
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        /* Timeout. */
        tv.tv_sec = 0;
        tv.tv_usec = 0;

        r = select(fd + 1, &fds, NULL, NULL, &tv);
    } while ((r == -1 && (errno = EINTR)));
    if (r == -1) {
        state = STARTED;
        return NULL;
    }

    //CLEAR(buf);
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    xioctl(fd, VIDIOC_DQBUF, &buf);
    unsigned char *buffers_ = (unsigned char*)buffers[buf.index].start;
    xioctl(fd, VIDIOC_QBUF, &buf);
    state = STARTED;
    return buffers_;
}

int Camera::xioctl(int fh, int request, void *arg)
{
    int r = -1;
    do {
        r = v4l2_ioctl(fh, request, arg);
    } while (r == -1 && ((errno == EINTR) || (errno == EAGAIN)));

    if (r == -1) {
        throw std::string("Error " + errno);
        return 1;
    }
    return 0;
}

