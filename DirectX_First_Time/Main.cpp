#include <Windows.h>
#include <windowsx.h>
#include <d3d11.h>
#include <D3DX10.h>
#include <D3DX11.h>

#pragma region Global Variables
IDXGISwapChain* swapChain;
ID3D11Device* dev;
ID3D11DeviceContext* devcon;

ID3D11RenderTargetView* backbuffer;

ID3D11VertexShader *pVS;
ID3D11PixelShader *pPS;

ID3D11Buffer *pVBuffer;
ID3D11Buffer *iBuffer;
ID3D11InputLayout *pLayout;

#define SCREEN_WIDTH	1280
#define SCREEN_HEIGHT	720
#pragma endregion

#pragma region Function Prototypes
//Function prototypes
LRESULT CALLBACK WindowProc(HWND hWnd,
							UINT message,
							WPARAM wParam,
							LPARAM lParam);
void InitD3D(HWND hWnd);
void CleanD3D(void);
void RenderFrame(void);
void InitPipeline(void);
void InitGraphics(void);
#pragma endregion

struct VERTEX
{
	FLOAT X, Y, Z;
	D3DXCOLOR Color;
};

#pragma region Main
int WINAPI WinMain(HINSTANCE hInstance,
				   HINSTANCE hPrevInstance,
				   LPSTR lpCmdLine,
				   int nCmdShow)
{
	//Handler for the window
	HWND hWnd;

	//-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//
	//Registering a window
	//

	//Struct which holds window information
	WNDCLASSEX wc;

	//clears of the window class for use
	//Basically initializes all of the data in wc to NULL
	ZeroMemory(&wc, sizeof(WNDCLASSEX));

	//Filling in all of the data
	wc.cbSize = sizeof(WNDCLASSEX);							//Allocates enough space in memory
	wc.style = CS_HREDRAW | CS_VREDRAW;						//Tells Windows to redraw the window if moved Horizontally or Vertically. Useless for full screen applications
	wc.lpfnWndProc = WindowProc;							//Tells the window class what function to call when it receives a message from Windows
	wc.hInstance = hInstance;								//Gives the class a handle to our application
	wc.hCursor = LoadCursor(NULL, IDC_ARROW);				//Stores the default mouse image for our cursor in the application
	//wc.hbrBackground = (HBRUSH) COLOR_WINDOW;				//Contains the brush that will color our window's background. Not used in Full Screen mode
	wc.lpszClassName = L"WindowClass1";						//This is the name of the window class

	RegisterClassEx(&wc);									//Registers the class with the above information

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//
	//Creating a window
	//

	//Adjusts the window/client size
	RECT wr = {0, 0, 500, 400};
	AdjustWindowRect(&wr,									//LPRECT lpRect:	A pointer to a RECT struct
		WS_OVERLAPPEDWINDOW,								//DWORD dwStyle:	Window style
		FALSE);												//BOOL bMenu:		Tells the function whether or not we're using menus
	
	//Creates a window and saves a handle to it
	hWnd = CreateWindowEx(NULL,								//DWORD dwExStyle:		Adds more options for the window style (extension)
		L"WindowClass1",									//LPCTSTR lpClassName:	The name of our window class
		L"Our First Window Application",					//LPCTSTR lpWindowName:	The name of the window (displayed in the title)
		WS_OVERLAPPEDWINDOW,								//DWORD dwStyle:		Adds more options for the window style
		300,												//int x:				X-Position of the window
		300,												//int y:				Y-Position of the window
		SCREEN_WIDTH,										//int nHeight:			Initial width of the window
		SCREEN_HEIGHT,										//int nHeight:			Initial height of the window
		NULL,												//HWND nWndParent:		Tells Windows what parent window created this window. NULL for desktop
		NULL,												//HMENU hMenu:			Handle to a menu bar
		hInstance,											//HINSTANCE hInstance:	Handle to the instance
		NULL);												//LPVOID IpParam:		Used only when creating multiple windows

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//
	//Display the window
	//

	//Shows the window! YAY
	ShowWindow(hWnd, nCmdShow);

	//Set up and initialize Direct3D
	InitD3D(hWnd);

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//
	//Handling Events and Messages
	//
	MSG msg;

	//Old way to do this, works fine in Windows programming but not with DirectX
	/*while(GetMessage(&msg,								//Wait for a message, then store it in msg
		NULL,												//HWND hWnd:			Handle to the window where the message is coming from. NULL means any window
		0,													//UINT wMsgFilterMin:	Limit the types of message to receive. 0 for both means any message.
		0))													//UINT wMsgFilterMax	Limit the types of message to receive. 0 for both means any message.
	{
		TranslateMessage(&msg);								//Translate the message into the right format
		
		DispatchMessage(&msg);								//Send it to the WindowProc function (below)
	}
	*/

	while(true)
	{
		if(PeekMessage(&msg,								//Same as above
			NULL,											//...
			0,												//...
			0,												//...
			PM_REMOVE))										//UINT wRemoveMsg: Takes either PM_REMOVE or PM_NOREMOVE
		{
			TranslateMessage(&msg);

			DispatchMessage(&msg);

			if(msg.message == WM_QUIT)
			{
				break;
			}
		}
		else
		{
			RenderFrame();
		}
	}

	//Cleans up and Shuts down DirectX
	CleanD3D();

	return msg.wParam;						
}
#pragma endregion

#pragma region Event Handling
//Called to handle messages
LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)	//Parameters are MSG struct values
{
	switch (message)
	{
	case WM_DESTROY:										//If a destroy message was sent...
		{
			PostQuitMessage(0);								//Close the application!
			return 0;
		}break;
	default:
		break;
	}

	return DefWindowProc(hWnd, message, wParam, lParam);	//Catches any messages we missed and sends them through the process
}
#pragma endregion

#pragma region Initialization and Shutdown
//
//Initialization
//
void InitD3D(HWND hWnd)
{
	//
	//Initialization of Direct3D
	//
	DXGI_SWAP_CHAIN_DESC scd;										//Struct which holds information about the swap chain

	ZeroMemory(&scd, sizeof(DXGI_SWAP_CHAIN_DESC));					//Prepares memory for scd to use

	scd.BufferCount = 1;											//Designates one back buffer
	scd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;				//Use 32-bit color
	scd.BufferDesc.Width = SCREEN_WIDTH;							//Sets the width of the buffer
	scd.BufferDesc.Height = SCREEN_HEIGHT;							//Sets the height of the buffer
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;				//How the back buffer will be used
	scd.OutputWindow = hWnd;										//The window to be used
	scd.SampleDesc.Count = 4;										//How many multisamples will be used (anti-aliasing)
	scd.Windowed = TRUE;											//Windowed/FullScreen mode
	scd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;						//Most efficient presentation method
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;				//Allows switching between full screen / windowed modes

	D3D11CreateDeviceAndSwapChain(									//Creates the Device and Swap Chain (wow, really? who would have guessed?)
		NULL,														//IDXGIAdapter *pAdapter:					Indicates the graphics adapter to use (referring to the GPU) NULL means default
		D3D_DRIVER_TYPE_HARDWARE,									//D3D_DRIVER_TYPE DriverType:				Determines whether Direct3D uses hardware or software for rendering
		NULL,														//HMODULE Software:							Sets the software to use if the above parameter dictates software
		NULL,														//UINT Flags:								Can take a few flags to alter how Direct3D runs
		NULL,														//D3D_FEATURE_LEVEL *pFeatureLevels:		Tells Direct3D what features of DirectX11 your program will work with
		NULL,														//UINT FeatureLevels:						Indicates the number of feature levels you included in the above list
		D3D11_SDK_VERSION,											//UINT SDKVersion:							Tells DirectX what version you developed for. Always the same for DirectX11 programs.
		&scd,														//DXGI_SWAP_CHAIN *pSwapChain:				Points the the swap chain description
		&swapChain,													//IDXGISwapChain **ppSwapChain:				Points to our pointer of our swap chain. Also will create the object! YAY!
		&dev,														//ID3D11Device **ppDevice:					Points to our pointer of our device. 
		NULL,														//D3D_FEATURE_LEVEL *pFeatureLevel:			Lets the programmer know what hardware is available after the function runs.
		&devcon);													//ID3D11DeviceContext **ppDeviceContext:	Points to our pointer of our device context.

	//---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//
	//Sets the render target
	//
	
	ID3D11Texture2D *pBackBuffer;
	swapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer);	//Finds the back buffer and creates the pBackBuffer Texture Object on that buffer
																				//Param 1: The number of the back buffer (0 base system)
																				//Param 2: The ID of the Texture2D Object. __uuidof() gets this ID
																				//Param 3: A void pointer to the object to make on the buffer. Many types of objects can be used here

	dev->CreateRenderTargetView(pBackBuffer, NULL, &backbuffer);				//Creates our render target object. 
																				//Param 1: Pointer to a texture. 
																				//Param 2: A Struct that describes the render target. NULL for the back buffer
	pBackBuffer->Release();														//Param 3: Address of the object pointer.
																				
	devcon->OMSetRenderTargets(1, &backbuffer, NULL);							//Actually sets our render Targets
																				//Param 1: The number of targets to set
																				//Param 2: A pointer to a list of the render targets
																				//Param 3: NULL for now, will be explained later

	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//
	//Set the depth/stencil buffer and view
	//
	D3D11_TEXTURE2D_DESC td;													//Description for our buffer

	ZeroMemory(&td, sizeof(D3D11_TEXTURE2D_DESC));
																				
	td.Width = SCREEN_WIDTH;													
	td.Height = SCREEN_HEIGHT;													
	td.MipLevels = 1;															
	td.ArraySize = 1;															
	td.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;									
	td.SampleDesc.Count = 4;													
	td.Usage = D3D11_USAGE_DEFAULT;												
	td.BindFlags = D3D11_BIND_DEPTH_STENCIL;									
	td.CPUAccessFlags = 0;														
	td.MiscFlags = 0;															

	//----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
	//
	//Sets the viewport
	//

	D3D11_VIEWPORT viewport;						//Struct which holds viewport information
	ZeroMemory(&viewport, sizeof(D3D11_VIEWPORT));

	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width	  =	SCREEN_WIDTH;
	viewport.Height   =	SCREEN_HEIGHT;

	devcon->RSSetViewports(1, &viewport);			//Activates viewport structs

	InitPipeline();
	InitGraphics();
}

//
//Shutdown
//
void CleanD3D(void)
{
	swapChain->SetFullscreenState(FALSE, NULL);						//Switch to windowed mode before shutdown

	pVS->Release();
	pPS->Release();
	swapChain->Release();											//MUCH easier to cleanup!!
	backbuffer->Release();
	dev->Release();													//MUCH easier to cleanup!!
	devcon->Release();												//MUCH easier to cleanup!!

	//
	//NOTE: If you don't release things they will continue to run even after the program closes!! THIS IS BAD
	//
}
#pragma endregion

#pragma region Rendering
void RenderFrame(void)
{
	devcon->ClearRenderTargetView(backbuffer, D3DXCOLOR(0.0f, 0.2f, 0.4f, 1.0f));		//Clear the back buffer to the given color

		UINT stride = sizeof(VERTEX);
		UINT offset = 0;
		devcon->IASetVertexBuffers(			   //Tells the GPU which vertices to read from when rendering
			0,								   //UINT StartSlot:					Advanced
			1,								   //UINT NumBuffers:					Number of Buffers we're setting
			&pVBuffer,						   //ID3D11Buffer **ppVertexBuffers:	A pointer to an array of vertex buffers (we only have one)
			&stride, 						   //UINT *pStrides:					A pointer to an array of vertex sizes (we only have one)
			&offset);						   //UINT *pOffsets:					A pointer to an array which tells the # of bytes we should start rendering from

		//Tells Direct3D to render triangles
		devcon->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		//Draws 3 vertices starting from vertex 0
		devcon->Draw(3, 0);

	swapChain->Present(0,0);															//Handles the swap
}

void InitPipeline(void)
{
	//
	//Load and Compile the two shaders from the .hlsl file
	//
	ID3D10Blob *VS, *PS;
	D3DX11CompileFromFile(					//Compiles a shader from a file containing it
		L"shaders.hlsl",					//LPCTSTR pSrcFile:				file containing the
		0,									//D2D10_SHADER_MACRO *pDefines:	advanced
		0,									//LPD3D10INCLUDE pInclude:		advanced
		"main",								//LPCSTR pFunctionName:			name of the shader's starting function
		"vs_5_0",							//LPCSTR pProfile:				profile of the shader (what type it is and what version to compile to)
		0,									//UINT Flags1:					advanced
		0,									//UINT Flags2:					advanced
		0,									//ID3DX11ThreadPump *pPump:		advanced
		&VS,								//ID3D10Blob **ppShader:		blob containing the compiled shader
		0,									//ID3D10Blob **ppErrorMsgs:		advanced
		0);									//HRESULT *pHResult:			advanced

	D3DX11CompileFromFile(L"shaders.hlsl", 0, 0, "PShader", "ps_5_0", 0, 0, 0, &PS, 0, 0);

	//
	//Encapsulate both shaders into shader objects
	//
	dev->CreateVertexShader(VS->GetBufferPointer(), VS->GetBufferSize(), NULL, &pVS);				//buffer pointer, buffer size, advanced, shader object
	dev->CreatePixelShader(PS->GetBufferPointer(), PS->GetBufferSize(), NULL, &pPS);

	//
	//Set both shaders to be active
	//
	devcon->VSSetShader(pVS, 0, 0);		//Shader object, advanced, advanced
	devcon->PSSetShader(pPS, 0, 0);

	//
	//Verifying the Input Layout
	//
	D3D11_INPUT_ELEMENT_DESC ied[] = 
	{
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		//Semantic:			A string whch tells the GPU what the value is used for
		//Semantic Index:	For multiple data values of the same semantic
		//Data Format:		Self-Explanatory. Must match what you use in the vertex
		//Input Slot:		Advanced
		//BytesIn:			# of bytes into the struct this element it
		//Element Use:		What the element is used as
		//Element Use Flag:	Not used with D3D11_INPUUT_PER_VERTEX_DATA
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};
	
	dev->CreateInputLayout(								//Creates an object representing the vertex format
		ied,											//D3D11_INPUT_ELEMENT_DESC *pInputElementDescs: Pointer to the element description array
		2,												//UINT NumElements:								The number of elements in the array
		VS->GetBufferPointer(),							//void *pShaderBytecodeWithInputSignature:		A pointer to the first shader in the pipeline
		VS->GetBufferSize(),							//SIZE_T BytecodeLength:						Length of the shader file
		&pLayout);										//ID3D11InputLayout **pInputLayout:				Pointer to the input layout object
	devcon->IASetInputLayout(pLayout);
}

void InitGraphics(void)
{
	//
	//Initialize the Vertex Buffer
	//
	VERTEX Vertices[] = 
	{
		{ 0.0f,	  0.5f, 0.0f, D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f)},
		{ 0.45f, -0.5f, 0.0f, D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f)},
		{-0.45f, -0.5f, 0.0f, D3DXCOLOR(0.0f, 0.0f, 1.0f, 1.0f)}
	};

	D3D11_BUFFER_DESC bd;
	ZeroMemory(&bd, sizeof(D3D11_BUFFER_DESC));

	bd.Usage = D3D11_USAGE_DYNAMIC;					//Write access by both CPU and GPU
	bd.ByteWidth = sizeof(VERTEX) * 3;				//Size of the buffer should be size of the VERTEX struct * 3
	bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;		//Set the buffer to be a vertex buffer
	bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;		//Allow the CPU to write in the buffer

	dev->CreateBuffer(&bd, NULL, &pVBuffer);		//Creates the buffer!

	//
	//Fill in the buffer with our vertex data!
	//
	D3D11_MAPPED_SUBRESOURCE ms;
	devcon->Map(pVBuffer, NULL, D3D11_MAP_WRITE_DISCARD, NULL, &ms);		//Map the buffer (allows access to it)
	memcpy(ms.pData, Vertices, sizeof(Vertices));							//Copy the data to the buffer
	devcon->Unmap(pVBuffer, NULL);											//Unmap the buffer

	//
	// Initialize the index buffer
	//
	UINT indices[] = {0, 1, 2};

	D3D11_BUFFER_DESC ibd;
	ZeroMemory(&ibd, sizeof(D3D11_BUFFER_DESC));

	ibd.Usage = D3D11_USAGE_IMMUTABLE;
	ibd.ByteWidth = sizeof(UINT) * 3;
	ibd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	ibd.CPUAccessFlags = 0;
	dev->CreateBuffer(&ibd, NULL, &iBuffer);
}
#pragma endregion