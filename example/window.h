#ifndef _WINDOW_H_
#define _WINDOW_H_


#include <gtkmm/window.h>
#include <gtkmm.h>

#include <memory>
#include <iostream>
class Window: public Gtk::Window 
{
    public:
        Window();
        ~Window();
        void setImageSize(int x, int y);
        typedef void (*callback)();
        typedef void (*path_callback)(const char *path);
        void addCallbacks(callback c1, callback c2, callback c3, path_callback c4);
        /*! Recive an Image
         *
         * \param image the image to show
         */
        void getNewImage(unsigned char *image2);
    private:
        callback continous, stop, once;
        path_callback change_camera;
        Gtk::Image *interactive_image;
        Glib::RefPtr<Gdk::Pixbuf> pixbuf1, pixbuf2;
        Gtk::Button *stop_button;

        Gtk::Button *cont_button;
        Gtk::Button *grab_button;
        Gtk::ComboBoxText *camera_select;
        Gtk::Grid grid;

        void stopTaking ();
        void takeOnce ();
        void continueTaking ();
        void changeCamera();

        struct image_parameters{
            bool init = false;
            int x;
            int y;
        } image;
};

#endif // _WINDOW_H_
