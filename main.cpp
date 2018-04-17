#include <iostream>

#include <cv.h>
#include <highgui.h>

#include <vtkNew.h>
#include <vtkSmartPointer.h>
#include <vtkImageActor.h>
#include <vtkImageImport.h>
#include <vtkImageData.h>
#include <vtkImageFlip.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderWindow.h>
#include <vtkRenderer.h>
#include <vtkCommand.h>

//!IplImage to vtkImageData
/**
 * Transforms data from cv::Mat to vtkImageData. It assumes
 * that parameters are correctly initialized.
 *
 * \param src OpenCV matrix representing the image data (source)
 *
 * \param dst vtkImageData representing the image (destiny)
 *
 */
void Ipl2VTK(const cv::Mat& src, const vtkSmartPointer<vtkImageData>& dst)
{
    cv::Mat src_rgb;
    cv::cvtColor( src, src_rgb, CV_BGR2RGB );

    vtkSmartPointer<vtkImageImport> importer = vtkSmartPointer<vtkImageImport>::New();

    //importer->SetOutput( dst );
    importer->SetDataSpacing( 1, 1, 1 );
    importer->SetDataOrigin( 0, 0, 0 );
    importer->SetWholeExtent(   0, src_rgb.size().width-1, 0, src_rgb.size().height-1, 0, 0 );
    importer->SetDataExtentToWholeExtent();
    importer->SetDataScalarTypeToUnsignedChar();
    importer->SetNumberOfScalarComponents( src_rgb.channels() );
    importer->SetImportVoidPointer( src_rgb.data );
    importer->Update();

    vtkSmartPointer<vtkImageFlip> imgFlip = vtkSmartPointer<vtkImageFlip>::New();
    imgFlip->SetInputData( importer->GetOutput());
    imgFlip->SetFilteredAxis(1);
    imgFlip->SetOutput(dst);
    imgFlip->Update();

    return;
}

//!Class vtkTimerCallback
/**
 * This class add support for timing events
 */
class vtkTimerCallback : public vtkCommand
{

public:
    vtkTimerCallback():frame(0){}
    ~vtkTimerCallback(){};

public:
    static vtkTimerCallback *New()
    {
        vtkTimerCallback *cb = new vtkTimerCallback;
        cb->TimerCount = 0;
        return cb;
    }

    virtual void Execute(vtkObject *vtkNotUsed(caller), unsigned long eventId,
                       void *vtkNotUsed(callData))
    {
        if (vtkCommand::TimerEvent == eventId)
        {
            ++this->TimerCount;
        }
        cout << this->TimerCount << endl;
        frame = cvQueryFrame(capture);
        cv::Mat imageMatrix(frame,true);
        Ipl2VTK(imageMatrix, imageData);
        window->Render();

    }

    void SetCapture(CvCapture *cap)
    {
        capture = cap;
    }

    void SetImageData(vtkImageData *id)
    {
        imageData = id;
    }

    void SetActor(vtkImageActor *act)
    {
        actor = act;
    }

    void SetRenderer(vtkRenderer *rend)
    {
        renderer = rend;
    }

    void SetRenderWindow(vtkRenderWindow *wind)
    {
        window = wind;
    }

private:
    int TimerCount;
    IplImage *frame;
    CvCapture *capture;
    vtkSmartPointer<vtkImageData> imageData;
    vtkSmartPointer<vtkImageActor> actor;
    vtkSmartPointer<vtkRenderer> renderer;
    vtkSmartPointer<vtkRenderWindow> window;
};

//!Entry point
int main(int argc, char *argv[])
{
    IplImage* frame = nullptr;
    CvCapture* capture = nullptr;

    capture = cvCaptureFromCAM(0);
    if(!capture)
    {
        std::cout << "Unable to open the device" << std::endl;
        exit(EXIT_FAILURE);
    }

    frame = cvQueryFrame(capture);
    if(!frame)
    {
        std::cout << "Unable to get frames from the device" << std::endl;
        cvReleaseCapture(&capture);
        exit(EXIT_FAILURE);
    }

    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    cv::Mat imageMatrix(frame,true);

    Ipl2VTK( imageMatrix, imageData );

    vtkSmartPointer<vtkImageActor> actor = vtkSmartPointer<vtkImageActor>::New();
    actor->SetInputData(imageData);

    vtkSmartPointer<vtkRenderer> renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> renderWindow = vtkSmartPointer<vtkRenderWindow>::New();
    renderWindow->AddRenderer(renderer);

    vtkSmartPointer<vtkRenderWindowInteractor> renderInteractor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
    renderInteractor->SetRenderWindow(renderWindow);
    renderInteractor->Initialize();
    renderInteractor->CreateRepeatingTimer(30);

    renderer->AddActor(actor);
    renderer->SetBackground(1,1,1);
    renderer->ResetCamera();

    vtkSmartPointer<vtkTimerCallback> timer = vtkSmartPointer<vtkTimerCallback>::New();
    timer->SetImageData(imageData);
    timer->SetCapture(capture);
    timer->SetActor(actor);
    timer->SetRenderer(renderer);
    timer->SetRenderWindow(renderWindow);
    renderInteractor->AddObserver(vtkCommand::TimerEvent, timer);

    renderInteractor->Start();

    cvReleaseCapture(&capture);

    return 0;
}
