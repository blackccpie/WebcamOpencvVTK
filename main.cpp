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

#include <mutex>
#include <thread>
#include <iostream>

// see : https://www.vtk.org/pipermail/vtkusers/2014-July/084639.html
// for more inspiration

class MyVtkVideoRender
{
public:
	MyVtkVideoRender( int sizeX, int sizeY )
	{
		m_image_data->SetDimensions( sizeX, sizeY, 1 );
		m_image_data->AllocateScalars(VTK_UNSIGNED_CHAR,3);
		m_actor->SetInputData( m_image_data );
		m_render_window->AddRenderer( m_renderer );
		m_render_interactor->SetRenderWindow( m_render_window );
		m_render_interactor->Initialize();
		m_render_interactor->CreateRepeatingTimer(30);
		m_renderer->AddActor( m_actor );
		m_renderer->SetBackground(1,1,1);
		m_renderer->ResetCamera();
		m_timer->SetImageData( m_image_data );
		m_timer->SetRenderWindow( m_render_window );
		m_render_interactor->AddObserver( vtkCommand::TimerEvent, m_timer );
	}
	void Start()
	{
		m_render_interactor->Start();
	}
	void SetImage( const cv::Mat& image )
	{
		m_timer->SetImage( image );
	}
private:
	/**
	 * This internal private class add support for timing events
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

		void Execute(   vtkObject* vtkNotUsed(caller),
						unsigned long eventId,
						void* vtkNotUsed(callData))
		{
			if ( vtkCommand::TimerEvent == eventId )
			{
				std::lock_guard<std::mutex> lock( m_mutex );
				_mat2VTK();
				m_render_window->Render();

				static std::chrono::time_point<std::chrono::system_clock> last, current;

				current = std::chrono::system_clock::now();

				auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>( current - last ).count();

				std::cout << "FPS : " << 1000 / elapsed_ms << std::endl;

				last = current;
			}
		}

		void SetImage( const cv::Mat& image )
		{
			std::lock_guard<std::mutex> lock( m_mutex );
			cv::cvtColor( image, m_image, CV_BGR2RGB );
		}

		void SetImageData( vtkSmartPointer<vtkImageData> data )
		{
			m_image_data = data;
		}

		void SetRenderWindow( vtkSmartPointer<vtkRenderWindow> win )
		{
			m_render_window = win;
		}

		void SetActor(vtkImageActor*){/*NOTHING TO DO YET*/}
		void SetRenderer(vtkRenderer*){/*NOTHING TO DO YET*/}

	private:
		inline void _mat2VTK()
		{
			vtkSmartPointer<vtkImageImport> importer = vtkSmartPointer<vtkImageImport>::New();
			importer->SetDataSpacing( 1, 1, 1 );
			importer->SetDataOrigin( 0, 0, 0 );
			importer->SetWholeExtent( 0, m_image.size().width-1, 0, m_image.size().height-1, 0, 0 );
			importer->SetDataExtentToWholeExtent();
			importer->SetDataScalarTypeToUnsignedChar();
			importer->SetNumberOfScalarComponents( m_image.channels() );
			importer->SetImportVoidPointer( m_image.data );
			importer->Update();

			vtkSmartPointer<vtkImageFlip> img_flip = vtkSmartPointer<vtkImageFlip>::New();
			img_flip->SetInputData( importer->GetOutput());
			img_flip->SetFilteredAxis(1);
			img_flip->SetOutput( m_image_data );
			img_flip->Update();

			return;
		}

	private:
		std::mutex m_mutex;
		int m_timer_count = 0;
		cv::Mat m_image;
		vtkSmartPointer<vtkImageData> m_image_data;
		vtkSmartPointer<vtkRenderWindow> m_render_window;
	};

private:
	vtkSmartPointer<vtkImageData> m_image_data = vtkSmartPointer<vtkImageData>::New();
	vtkSmartPointer<vtkImageActor> m_actor = vtkSmartPointer<vtkImageActor>::New();
	vtkSmartPointer<vtkRenderer> m_renderer = vtkSmartPointer<vtkRenderer>::New();
    vtkSmartPointer<vtkRenderWindow> m_render_window = vtkSmartPointer<vtkRenderWindow>::New();
	vtkSmartPointer<vtkRenderWindowInteractor> m_render_interactor = vtkSmartPointer<vtkRenderWindowInteractor>::New();
	vtkSmartPointer<vtkTimerCallback> m_timer = vtkSmartPointer<vtkTimerCallback>::New();
};

bool g_run = true;

//!Entry point
int main(int argc, char *argv[])
{
	   MyVtkVideoRender my_renderer( 1280, 720 );

	std::thread t([&my_renderer]()
	{
		cv::Mat image_mat;
		cv::VideoCapture capture(0);

		if( !capture.isOpened() )
		{
			std::cout << "Unable to open the device" << std::endl;
			exit(EXIT_FAILURE);
		}

		while(g_run)
		{
			capture >> image_mat;
			my_renderer.SetImage( image_mat );
		}
	});

	my_renderer.Start();

	g_run = false;
	t.join();

    return 0;
}
