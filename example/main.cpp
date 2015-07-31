/*
 * This example show how to use v4l2_pp library to display webcam stream on Gtk Image widget
 */

#include <iostream>
#include <thread>
#include "../v4l2_pp/v4l2_camera.h"
#include "window.h"

using namespace std;

V4L2::Camera camera;

int allow_frame = CAMERA_ASYNC_CONTINUE;
Window *window;

/**
 * Sets canvas size to camera output size
 */
void setImageToCamera(V4L2::Camera *camera)
{
    try {
        int x=0, y=0;
        camera->getSize(&x, &y);
        window->setImageSize(x, y);
    }catch(...){}
}

void continous(){
    allow_frame = CAMERA_ASYNC_CONTINUE;
}

void stop(){
    allow_frame = CAMERA_ASYNC_STOP;
}

void get_one(){
    unsigned char *imag = camera.getImage();
    if(imag != NULL)
        window->getNewImage(imag);
}

void change_camera(const char *path){
    allow_frame = CAMERA_ASYNC_STOP;
    std::this_thread::sleep_for (std::chrono::milliseconds(200));
    int ret = camera.stopCapturing();
    if(ret != 0)
        return;

    std::this_thread::sleep_for (std::chrono::milliseconds(20));
    ret = camera.close();
    if(ret != 0)
        return;

    camera.setDevice(path);
    ret = camera.open();
    cout<<"Open "<<(ret==2?"Cannot open - bad device":ret==0?"Opened":"Error")<<ret<<endl;
    if(ret != 0)
        return;

    camera.startCapturing();
    allow_frame = CAMERA_ASYNC_CONTINUE;
}

int main(int argc, char **argv)
{
    //Prepare GUI part
    Gtk::Main kit(argc, argv);
    window = new Window();
    window->addCallbacks(&continous, &stop, &get_one, &change_camera);

    //Set device node
    camera.setDevice("/dev/video0");

    //Open device
    int ret = camera.open();
    if(ret != V4L2::CAMERA_SUCCESS)
        cout<<"Error while initialize";

    //Set size of images
    ret = camera.setSize(1280, 720);
    if(ret != V4L2::CAMERA_SUCCESS)
        cout<<"Error while changing size";
    setImageToCamera(&camera);

    
    bool run_loop = true;
    //Run in new thread to allow gtk working
    std::thread t1 ([&](){
        //Start camera 
        ret = camera.startCapturing();
        if(ret != V4L2::CAMERA_SUCCESS)
            cout<<"Error while starting capturing"<<ret;

        //Example 1
        //Get single image
        unsigned char *imag = camera.getImage();
        //Put image into window
        window->getNewImage(imag);

        //Stop camera to be able to change size
        ret = camera.stopCapturing();
        if(ret != V4L2::CAMERA_SUCCESS)
            cout<<"Camera stop failed "<<ret<<endl;

        //Resize widget to camera size
        setImageToCamera(&camera);

        //Sleep to see result of first example
        std::this_thread::sleep_for(std::chrono::seconds(1));

        //Start camera
        ret = camera.startCapturing();

        //Example 2:
        //Receive images asynchronously, passing lambda
        while(run_loop){
            if(allow_frame != CAMERA_ASYNC_STOP){
               camera.getImagesSynchronously(
                    [&](unsigned char *imag){
                        if(allow_frame == CAMERA_ASYNC_STOP)//This code shouldn't have been needed. To fix
                            return CAMERA_ASYNC_STOP;

                        if(imag != NULL)
                            window->getNewImage(imag);

                        return allow_frame;
                    });
            }
            std::this_thread::sleep_for (std::chrono::milliseconds(1000));
        }
        camera.stopCapturing();
    });

    //Run gtk window
    kit.run(*window);

    //Stop receiving images from camera
    allow_frame = CAMERA_ASYNC_STOP;
    run_loop = false;
    t1.join();
    camera.close();
    delete(window);
}
