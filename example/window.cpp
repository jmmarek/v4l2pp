#include "window.h"

Window::Window(){
    set_border_width(10);
    set_default_size(1000,600);

    interactive_image = new Gtk::Image();
    add(grid);
    stop_button = new Gtk::Button("Stop");

    grab_button = new Gtk::Button("Grab");

    cont_button = new Gtk::Button("Continous");

    camera_select = new Gtk::ComboBoxText (true);
    camera_select->append("/dev/video0");
    camera_select->append("/dev/video1");

    grid.insert_column(2);
    grid.attach(*interactive_image, 0, 0, 4, 10);


    grid.attach(*stop_button, 0, 10, 1, 1);
    grid.attach(*cont_button, 1, 10, 1, 1);
    grid.attach(*grab_button, 2, 10, 1, 1);
    grid.attach(*camera_select, 3, 10, 1, 1);

    stop_button->signal_clicked().connect( sigc::mem_fun(*this, &Window::stopTaking));
    grab_button->signal_clicked().connect( sigc::mem_fun(*this, &Window::takeOnce));
    cont_button->signal_clicked().connect( sigc::mem_fun(*this, &Window::continueTaking));
    camera_select->signal_changed().connect(sigc::mem_fun(*this,&Window::changeCamera) );

    show_all();
}

void Window::addCallbacks(callback c1, callback c2, callback c3, path_callback c4)
{
    continous = c1;
    stop = c2;
    once = c3;
    change_camera = c4;
}

void Window::setImageSize(int x, int y)
{
    this->image.init = true;
    this->image.x = x;
    this->image.y = y;
}

void Window::changeCamera(){
    Glib::ustring path = camera_select->get_active_text();
    if(!(path.empty()))
        change_camera(path.c_str());
}

void Window::stopTaking (){
    stop();
}

void Window::takeOnce (){
    once();
}

void Window::continueTaking (){
    continous();
}

Window::~Window(){
    delete stop_button;
    delete cont_button;
    delete grab_button;
    delete camera_select;
    delete interactive_image;
}

void Window::getNewImage(char* image2){
    if(!image.init)
        throw std::string("Set image parameters first");

    if(image2 == NULL)
        return;

    pixbuf1 = Gdk::Pixbuf::create_from_data((guint8*)image2, Gdk::COLORSPACE_RGB, false, 8, image.x, image.y, image.x*3); 
    interactive_image->set(pixbuf1);
    interactive_image->show();
}
