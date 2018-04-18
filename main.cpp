#include <iostream>

#include <opencv2/opencv.hpp>

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

// see : https://www.vtk.org/pipermail/vtkusers/2014-July/084639.html
// for more inspiration

/**
 * Transforms data from cv::Mat to vtkImageData
 */
void ipl2vtk(const cv::Mat& src, const vtkSmartPointer<vtkImageData>& dst)
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

/**
 * This class add support for timing events
 */
class vtkTimerCallback final : public vtkCommand
{
public:
    vtkTimerCallback() {}
    ~vtkTimerCallback() {};

public:

    static vtkTimerCallback* New()
    {
        return new vtkTimerCallback();
    }

    void Execute(   vtkObject *vtkNotUsed(caller),
                    unsigned long eventId,
                    void *vtkNotUsed(callData))
    {
        if (vtkCommand::TimerEvent == eventId)
        {
            ++timerCount;
        }
        cout << timerCount << endl;
        *capture >> imageMatrix;
        ipl2vtk( imageMatrix, imageData );
        window->Render();

    }

    void SetCapture(cv::VideoCapture* cap)
    {
        capture = cap;
    }

    void SetImageData(vtkImageData *id)
    {
        imageData = id;
    }

    void SetActor(vtkImageActor *act)
    {
        //NOTHING TO DO YET
    }

    void SetRenderer(vtkRenderer *rend)
    {
        //NOTHING TO DO YET
    }

    void SetRenderWindow(vtkRenderWindow *wind)
    {
        window = wind;
    }

private:
    int timerCount = 0;
    cv::Mat imageMatrix;
    cv::VideoCapture* capture = nullptr;
    vtkSmartPointer<vtkImageData> imageData;
    vtkSmartPointer<vtkRenderWindow> window;
};

//!Entry point
int main(int argc, char *argv[])
{
    cv::Mat imageMatrix;

    cv::VideoCapture capture(1); // open the default camera
	capture.set(CV_CAP_PROP_FRAME_WIDTH , 640); 
	capture.set(CV_CAP_PROP_FRAME_HEIGHT , 480); 
	capture.set (CV_CAP_PROP_FOURCC, CV_FOURCC('B', 'G', 'R', '3'));

	if(!capture.isOpened())  // check if we succeeded
    {
        std::cout << "Unable to open the device" << std::endl;
        exit(EXIT_FAILURE);
    }

    capture >> imageMatrix;

    vtkSmartPointer<vtkImageData> imageData = vtkSmartPointer<vtkImageData>::New();
    ipl2vtk( imageMatrix, imageData );

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
    timer->SetCapture(&capture);
    timer->SetRenderWindow(renderWindow);
    renderInteractor->AddObserver(vtkCommand::TimerEvent, timer);

    renderInteractor->Start();

    return 0;
}
