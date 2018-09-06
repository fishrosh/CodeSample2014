// ////////////////////////////////////////////////////////
//
// C++ Code Sample
// Kamil Blaszczyk
// 2014
//
// This sample contains a set of classes
// designed to fit simple raytracer app.
// Classes (with the exception of class
// 'Space', which is currently under
// development) are provided with definitions
// of all member methods, constructors
// operators, destructors etc. 
// 
// Desriptions of the subsequent classes
// are provided along their declarations
// and definitions, dependant on where
// particular information fits best.
// Classes declarations' comment sections
// contains most important explanations
// for a particular class' tasks and
// purposes.
// 
// //////////////////////////////////////////////

#pragma comment ( lib, "d3d10.lib" )
#pragma comment ( lib, "d3dx10d.lib" )
#pragma comment ( lib, "d3dx9d.lib" )

#include <d3d10.h>
#include <D3DX10.h>
#include <xnamath.h>

#include <string>
#include <vector>
#include <memory>
#include <sstream>
#include <algorithm>

#define XMFLOAT_WSTREAM( f )	f.x << L" " << f.y << L" " << f.z
#define	ERRORMACRO( x )			MessageBox( NULL, x, L"Error macro", MB_OK )

class 	UserInput;
class 	Mateyko;
class 	ShaderInput;
class	Camera;
class	Space;
class 	Object3D;

struct	Timer;
struct 	Vertex;

// forward function declarations.
XMMATRIX	getSpaceMatrix( XMFLOAT3, XMFLOAT3, bool );

// ////////////////////////////////////////////////
// ///////////////////////////////////////////////
// //////////////////////////////////////////////
// 
// CLASSES
// 
// /////////////////////////////////////////
// ////////////////////////////////////////
// ///////////////////////////////////////

// UserInput is an interface type class
// that provides responses to the users actions
// for various derived classes. any class that
// is supposed to be controlable needs to implement
// those function in order to properly respond
// to user's actions.
class UserInput
{
public:

	// all methods are pure virtual even if 
	// not all classes use all of the functions.
	// in such case overloaded method just has
	// an empty body. most of the method's names
	// from the list below are self-explanatory
	// so there's no need to describe them further
	
	virtual void	MouseUpDown( double )		=0;		
	virtual void	MouseLeftRight( double )	=0;
	virtual void	ArrowsUpDown( double )		=0;
	virtual void	ArrowsLeftRight( double )	=0;
	virtual void	NmpdNumber( int )			=0;		// NUMPAD numbers
	virtual void	NmpdAddSubtract( double )	=0;		// NUMPAD add key ( VK_ADD ) and subtract ( VK_SUBTRACT )
	virtual void	WsadUpDown( double )		=0;		
	virtual void	WsadLeftRight( double )		=0;	
	
};

// class Mateyko is the main drawing-painting-rendering
// class, that holds all the directx components needed
// for displaying an image. those components are initialized
// by the InitDevice method. every instance of a class
// also has its std::vectors storing the data about objects
// on the scene. 
class Mateyko
{
	// vector of pointers to the objects of the scene, and then vector of theirs
	// colors. storing colors like that makes it easier to pass them to constant
	// buffer, and using pointers instead of actual objects makes program faster,
	// for as we know, std::vector likes to reallocate its content from time to time
	std::vector< std::shared_ptr< Object3D > >	objects;
	std::vector< XMFLOAT4 >						oColors;
	
	// variables for various parts of the engine
	ID3D10Device*				pd3dDevice;
	IDXGISwapChain*				pSwapChain;
	ID3D10RenderTargetView*		pRenderTargetView;
	ID3D10Texture2D*			pDepthStencil;
	ID3D10DepthStencilView*		pDepthStencilView;

	// other variables
	ID3D10ShaderResourceView*	FloorTextureRV;	
	D3D10_DRIVER_TYPE			pDriverType;
	UINT						Width, Height;

	// pointers to the various devices that manage events on the scene
	// they need to be manually binded, by default are set to NULL
	ShaderInput*				pInput;
	Camera*						pCam;
	Space*						pSpace;

	// a ground/floor object
	Object3D*					oGroundZero;
	
public:

	// standard constructors and assigment operator
	// remember the default constructors sets all devices' parts
	// to NULL, and to use it you need to initialize them via 
	// InitDevice method.
	Mateyko();
	Mateyko( const Mateyko& );
	Mateyko&	operator=( const Mateyko& );
	
	// destructor
	~Mateyko();
	
#ifdef HAS_MOVE_SEMANTICS
	// if compiler we use employ move semantics define
	// move constructor and move operator
	Mateyko( Mateyko&& );
	Mateyko&	operator=( Mateyko&& );
#endif

	HRESULT				InitDevice( HWND hWnd );			// initializes the device
	HRESULT				loadTexture( LPCWSTR szFileName );	// loads the texture for the floor
	ID3D10Device*		GetDevice();						// returns a pointer to the device, so other classes can use it (e.g. shader input)
	void				ReleaseMe();
	void				PaintScene();						// paints a scene

	// bind devices to the respective pointers
	// the PaintScene method won't work unless those device are set
	void				BindInput( ShaderInput* shi );
	void				BindCamera( Camera* cam );
	void				BindSpace( Space* spa );

	// those can create standard shapes - a flat rectangle surface and a sphere
	// of a desired number of parallels and meridians
	void				formSphere( LPCWSTR _name, UINT meridians, UINT parallels, float radius, XMFLOAT4 color );
	void				formRectangleObject( LPCWSTR _name, float length, float width, XMFLOAT3 planeNormal, XMFLOAT3 lenDir );

	// insert and remove methods.
	// responsible for adding new objects to the objects vector (and removing from it)
	void				InsertObject( Object3D* );
	void				InsertObject( void* verts, DWORD* inds, UINT vSize, UINT iSize, XMFLOAT4 colololo );
	void				RemoveObject( int oNum );
	void				RemoveAll();

	// remaining methods
	void				updateColor( unsigned int oNumber, XMFLOAT4 color );	// update color of a desired number
	void				GetClientRectSize( UINT& _width, UINT& _height );		// get the size of a client window
};

// //////////////////////////////////////////////
// 
// SHADER INPUT CLASS
// 
// /////////////////////////////////////////

// ShaderInput class is responsible for
// passing various shader data into the 
// shaders, as well as holding the Technique,
// Effect and Input Layout
class ShaderInput
	:	
	public UserInput
	// ShaderInput inherits the UserInput interface, 
	// so user may affect the way scene is rendered
{
	// pointer variables for various parts of a shader input.
	// those are set during the construction time
	ID3D10InputLayout*					Input;
	ID3D10Effect*						Effect;
	ID3D10EffectTechnique*				Technique;

	// all the variables below pass some data to the constant buffer
	// most basic matrices needed for proper work of the camera
	ID3D10EffectMatrixVariable*			World;
	ID3D10EffectMatrixVariable*			View;
	ID3D10EffectMatrixVariable*			Projection;

	// those vector arrays pass info about some of the
	// objects on the scene, so shader can access that data any time
	
	ID3D10EffectVectorVariable*			Light;				// position of lights
	ID3D10EffectVectorVariable*			CamEye;				// current camera position
	ID3D10EffectVectorVariable*			BigBalls;			// position of all of the spheres
	ID3D10EffectVectorVariable*			OColors;			// colors of those spheres

	// those two tells the shader how many objects are on the scene
	// and which number in the vector arrays above has the currently rendered object
	// (note: there's special -1 index for the floor/ground object)
	
	ID3D10EffectScalarVariable*			num_processed;
	ID3D10EffectScalarVariable*			count_processed;

	// shading control variables defines how the scene ought to
	// be rendered 
	
	ID3D10EffectScalarVariable*			brightness;			// sky brightness
	ID3D10EffectScalarVariable*			reflectance;		// level of reflectance of the spheres and the floor
	ID3D10EffectScalarVariable*			skybright;			// not used now	
	ID3D10EffectScalarVariable*			diffuseStr;			// strength of diffuse color
	ID3D10EffectScalarVariable*			gamma;				// gamma modifier
	ID3D10EffectScalarVariable*			ColorNumVar;		// defines which channel should be displayed - usefull for tests

	// textures and maps
	ID3D10EffectShaderResourceVariable*	FloorTexture;	
	
	// those variables hold the values that need to be passed
	// to shaders. names are the same, except for 'v' prefix
	
	float	vGamma;					// gamma, can be disabled
	float	vBrightness;			// overall brightness of a rendered image
	float	vReflectance;			// the level of reflectance on the spheres
	float	vSkyBrightness;			// sky brightness level, and since sky is the only source of light right now, the quantity of light on the scene
	float	vDiffusePower;			// how much diffuse color need to be amplified
	int		vChannel;				// number of channel that we want rendered
	
	// number of the variable that is currently susceptible
	// to the NmpdAddSubtract method influence.
	int		varIndex;
	
	// frames per second
	float	fps;

	// disabled constructors and assigment operator:
private:	ShaderInput();		
			ShaderInput( const ShaderInput& );
			ShaderInput&	operator=( const ShaderInput& );
public:

	// constructors: the default and copy constructors
	// were set private because they wouldn't
	// be able to initialize shader input's
	// basic variables without ID3D10Device
	// passed as an argument. same reason with
	// assigment operator.
	
	ShaderInput( ID3D10Device*	pd3dDevice, /* pointer to the device that will do the hard work */
		LPCWSTR szFileName, LPCSTR szTechName );
	
	// destructor
	~ShaderInput();

	// //////////////////////////////////////
	// methods:
	
	// next six methods prepares the shaders for the rendering
	// they are supposed to be called within PaintScene method
	// of a Mateyko class.
	
	void	PrepareShadingControlVars( int );			// passes shading control variables to shaders. int var is a count_processed
	void	PrepareCameraMatrices( float*, float* );	// passes camera matrices
	void	PrepareEyePos( float* );					// camera-eye position
	void	PreparePositions( float*, int );			// scene object's positions
	void	PrepareColors( float*, int );				// scene object's colors
	void	PrepareObject( float*, int );				// passes current object's world matrix and its index on the object list. 
	// invoked for every object separately
	
	void	SetFPS( float );							// sets fps variable
	void	SetFloorTex( ID3D10ShaderResourceView* );	// sets the resource view to floor's resource variable
	
	ID3D10EffectTechnique*		GetTech();	
	ID3D10InputLayout*			GetLayout();
	
	// methods inherited from UserInput interface.
	// only two are supposed to do something.
	// NmpdNumber sets which of the shader control variable
	// user is able to change using NmpdAddSubtract, 
	// e.g user may strenghten brightness or
	// reduce gamma to get better rendering results.
	// while NmpdNumber sets the discrete value
	// NmpdAddSubtract increases or decreases
	// values continously.
	
	void	NmpdNumber( int );
	void	NmpdAddSubtract( double );
	
	// remaining inherited methods shouldn't do anything:
	void	MouseUpDown( double )		{}		
	void	MouseLeftRight( double )	{}
	void	ArrowsUpDown( double )		{}
	void	ArrowsLeftRight( double )	{}
	void	WsadUpDown( double )		{}		
	void	WsadLeftRight( double )		{}

};

// input layout description, matches the Vertex struct
const D3D10_INPUT_ELEMENT_DESC 	vertex_desc[]  =	
{
	{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
};

// //////////////////////////////////////////////
// 
// CAMERA CLASS
// 
// /////////////////////////////////////////

// camera class. prepares matrices for rendering
// (view and projection) using stored Eye, At, Up
// vectors (xmfloats). those three are affected
// by inherited from UserInput methods, so 
// certain keyboard messages may conduct
// different kinds of movement.
class Camera
	:	
	public UserInput
	// camera uses UserInput interface, 
	// so it can be moved or rotated by user
{
	
	// three most basic vectors required to
	// construct the view matrix. eye is the
	// camera position, at the point camera is
	// focusing on, up is world's up direction
	
	XMFLOAT3	Eye;
	XMFLOAT3	At;
	XMFLOAT3	Up;
	
	float		FoV;				// field of view, required for projection matrix generation
	float		ScreenRatio;		// the ratio between the width and height of a client rectangle
	float		fps;				// frames per second, need that var to keep velocities at constant level
	float		acceleration;		// arrow keys movement is smoother if it starts slowly and then accelerates
	float		braking;			// if not being pushed by user camera slows down
	
	float		veloUpDown;			// velocity for up-down arrow keys. affected by acceleration
	float		veloLeftRight;		// velocity for left-right arrows. also affected by acceleration
	float		veloEyeRot;			// velocity for wsad left-right keys. not affected by acceleration
	float		veloAtRot;			// velocity for mouse left-right movement. not affected by acceleration
	
public:

	// constructors:
	Camera();
	
	// a special constructor in case we want camera being set 
	// in the specific spot from the beginning. 
	Camera( XMFLOAT3 _eye, XMFLOAT3 _at, XMFLOAT3 _up );
	
	// copy-cinstructor, destructor as well as assigment operator
	// may be auto-generated, as the class does not
	// contain anything that required special treatment
	// while deleting, copying or assigning
	
	// Camera( const Camera& );
	// Camera&	operator=( const Camera& );
	// ~Camera();
	
#ifdef HAS_MOVE_SEMANTICS
	// no move semantics, as this class contains
	// no pointers. move semantics is useless here
#endif	

	// three methods below are used by the PaintScene method
	// of a Mateyko class. they return current matrices
	// computed using the position and direction of the cam.
	
	XMMATRIX	GetProjection();
	XMMATRIX	GetView();
	XMFLOAT4	GetEyePos();
	
	// setters
	void		SetFPS( float );
	void		SetScreenRatio( float );
	void		SetFoV( float );
	
	// decreases the velocities by braking value divided by fps.
	// this guarantees steady fps-independent velocity decrease.
	// ought to be called every frame when velocities could
	// possibly increase (that means when methods inherited
	// from UserInput may be called)
	void		UpdateCam();
	
	// methods inherited from the UserInput interface
	// mouse movement rotates the camera around the At point
	// (you can think of it as similar to the cameras in some
	// isometric RPG games or strategies), while WSAD keys
	// rotates Camera around its Eye point, similar to the
	// first person perspective cameras. Arrows are responsible
	// for moving forward, backward, left and right. also,
	// that kind of movement is only along xz plane
	
	void	MouseUpDown( double );	
	void	MouseLeftRight( double );
	void	ArrowsUpDown( double );
	void	ArrowsLeftRight( double );
	void	WsadLeftRight( double );
	
	// those three are not used by Camera, so left them empty
	void	NmpdNumber( int )			{}
	void	NmpdAddSubtract( double )	{}
	void	WsadUpDown( double )		{}
};

// //////////////////////////////////////////////
// 
// OBJECT3D CLASS
// 
// /////////////////////////////////////////

class Object3D
{
	// buffer pointers for the drawing device
	// the initial idea was to keep them as shared_ptrs
	// but IUnknown interface from which buffers inherit
	// provide similar features.
	ID3D10Buffer*		vBuffer;
	ID3D10Buffer*		iBuffer;

	// size of the buffers, and its position
	// within the memory chunk passed to shdaers
	UINT				iSize, vSize;
	UINT				stride, offset;
	
private:	Object3D(); // yep everytime you call it, linker shouts
public:

	// Object3D construction requires pointer to the device
	// so it is impossible for the default construction to work
	// properly. therefore default constructor was set private
	// to avoid calling it

	Object3D( ID3D10Device*	_device, /* pointer to the device that will do the hard work */
		void* vertices, DWORD* indices, UINT _vSize, UINT _iSize );
	Object3D( const Object3D& );
	Object3D&	operator=( const Object3D& );
	
	// destructor:
	~Object3D();
	
#ifdef HAS_MOVE_SEMANTICS
	// if compiler we use employ move semantics define
	// move constructor and move operator
	Object3D( Object3D&& );
	Object3D&	operator=( Object3D&& );
#endif

	// nice methods:
	void	Draw( ID3D10Device* device, ID3D10EffectTechnique* tech );
};

// //////////////////////////////////////////////
// 
// STRUCTURES
// 
// /////////////////////////////////////////

// timer is a simple struct that holds the time got from
// windows GetTickCount func in its initialization time, 
// and calculates number of seconds that passed since that moment
struct Timer
{
	// since this is only small struct all methods and constructors
	// are defined here for the sake of simplicity
	
	// constructors, assign operator and destructor:
	Timer()
		: dwTimeStart( GetTickCount() )		{}
	Timer( const Timer& tim )
		:	dwTimeStart( tim.dwTimeStart )	{}
	Timer&	operator=( const Timer& tim )	{
		if( this != &tim )	{
			dwTimeStart = tim.dwTimeStart;
			return *this;
		}
	}
	~Timer()	{}
	
	// returns time in seconds that passed since the start of the timer
	float	GetTime()	{
		return ( GetTickCount() - dwTimeStart ) / 1000.0f;
	}
	
private: 	
	DWORD	dwTimeStart;	// keeps time Timer started
	
};

// represents a vertex in the 3d space
// with defined color and normal;
struct	Vertex
{
	XMFLOAT3	Pos;
	XMFLOAT3	Norm;
	XMFLOAT4	Color;
};

// ////////////////////////////////////////////////
// ///////////////////////////////////////////////
// //////////////////////////////////////////////
// 
// MATEYKO	:	METHODS, CONSTRUCTORS AND OPERATORS DEFINITIONS
// 
// /////////////////////////////////////////
// ////////////////////////////////////////
// ///////////////////////////////////////

// default constructor (and the only one, except copy)
// of the Mateyko class. it sets all members nullified
// those members can be set up via InitDevice method

Mateyko::Mateyko()
	:	pd3dDevice( NULL ),
		pSwapChain( NULL ),
		pRenderTargetView( NULL ),
		pInput( NULL ),
		pCam( NULL ),
		pSpace( NULL ),
		pDepthStencil( NULL ),
		pDepthStencilView( NULL ),
		oGroundZero( NULL ),
		FloorTextureRV( NULL ),
		Width( 0 ),
		Height( 0 )
		// note that there's no need for manual initialization
		// of vector members. therefore they are not mentioned here
{}

// copy constructor of the Mateyko class.
// it does NOT copy devices, textures or 
// shader vars. to use copied instance of
// this class it must be initialazed.
// it'll still share its cameras, input
// Space and sets of objects with the 
// instance it was copied from though.

Mateyko::Mateyko( const Mateyko& mat )
	:	pd3dDevice( NULL ),
		pSwapChain( NULL ),
		pRenderTargetView( NULL ),
		pDepthStencil( NULL ),
		pDepthStencilView( NULL ),
		oGroundZero( NULL ),
		FloorTextureRV( NULL ),
		
		// we do not allow copying devices.
		// every new instance of Mateyko class
		// must be initialazed anew via the
		// InitDevice method. therefore we have
		// to set all device's pointers to NULL
		
		pInput( mat.pInput ),
		pCam( mat.pCam ),
		pSpace( mat.pSpace ),
		
		// the exceptions are pointers of other objects
		// like Camera, Input and Space. we allow to copy 
		// those pointers, because they exist separately anyway
		
		objects( mat.objects.begin(), mat.objects.end() ),
		oColors( mat.oColors.begin(), mat.oColors.end() ),
		
		// other exceptions are std::vector sets of
		// objects and colors on the scene. those also
		// can be safely copied. (remember objects
		// is a vector of shared ptrs)
		
		Width( 0 ),
		Height( 0 )
		
		// those two will be set right when InitDevice
		// will be called. unless so, they're set to zero
		// in case GetClientRect will be called.
{}

// assigment operator of the Mateyko class
// does NOT assign directx related stuff
// only copy outer device's pointers and 
// the content of std::vectors. to use
// an instance after cpuying initialize
// it with InitDevice method.

Mateyko&	Mateyko::operator=( const Mateyko& mat )
{
	if( this != &mat )
	{
		// as was specified above in the copy constructor
		// comment section, we do not allow to copy devices
		// textures and other directx related stuff. it is
		// way simpler to code it that way safely,
		// and anyway we don't expect anyone
		// be juggling with plenty of drawing devices
		// just for the sake of it. in fact Mateyko
		// should have one, maybe up to few instances,
		// and there's hardly any scenario where
		// a direct copy of this class would be necessary.
		// therefore every time there's a need for
		// copying, copied instance must be initialized anew
		
		// first clean up the left operand
		if( pd3dDevice )			pd3dDevice->ClearState();
		if( pSwapChain )			pSwapChain->Release();
		if( pRenderTargetView )		pRenderTargetView->Release();
		if( pDepthStencil )			pDepthStencil->Release();
		if( pDepthStencilView )		pDepthStencilView->Release();
		if( FloorTextureRV )		FloorTextureRV->Release();
		if( oGroundZero )			delete oGroundZero;
		
		// clean up vectors
		objects.clear();
		oColors.clear();
		
		// set null to pd3dDevice
		pd3dDevice = NULL;
		
		// set null to screen proportions
		Width = 0;
		Height = 0;
		
		// assign outer devices. we do not need to erase
		// them, for they are only binded, not created
		// within the mateyko class
		pInput = mat.pInput;
		pCam = mat.pCam;
		pSpace = mat.pSpace;
		
		// insert the content of right operand's std::vectors
		// into the left's vectors, cleared a moment ago
		objects.insert( objects.begin(), mat.objects.begin(), mat.objects.end() );
		oColors.insert( oColors.begin(), mat.oColors.begin(), mat.oColors.end() );
		
		return *this;
	}
}

void	Mateyko::ReleaseMe()
{
	if( pd3dDevice )			pd3dDevice->ClearState();
	if( pSwapChain )			pSwapChain->Release();
	if( pRenderTargetView )		pRenderTargetView->Release();
	if( pDepthStencil )			pDepthStencil->Release();
	if( pDepthStencilView )		pDepthStencilView->Release();
	if( FloorTextureRV )		FloorTextureRV->Release();
	if( oGroundZero )			delete oGroundZero;
	if( pd3dDevice )			pd3dDevice->Release();
}

// class destructor
Mateyko::~Mateyko()
{
	if( pd3dDevice )			pd3dDevice->ClearState();
	if( pSwapChain )			pSwapChain->Release();
	if( pRenderTargetView )		pRenderTargetView->Release();
	if( pDepthStencil )			pDepthStencil->Release();
	if( pDepthStencilView )		pDepthStencilView->Release();
	if( FloorTextureRV )		FloorTextureRV->Release();
	if( oGroundZero )			delete oGroundZero;
	if( pd3dDevice )			pd3dDevice->Release();
	
	// note, we do not delete pointer to Camera, Space and Input.
	// those were binded here, not created, so we don't want to 
	// delete them.
}

// init device method of Mateyko class.
// calling this method is required for using Mateyko class.
// it creates the device, depth stencil and a swapchain
// defines render target view, also saves some info about
// the client rectangle. 

HRESULT		Mateyko::InitDevice( HWND hWnd )
{
	// hresult that will be returned in the end, or when sth go wrong
	HRESULT hr = S_OK;

	// finding client rectangle
	RECT rc;
	GetClientRect( hWnd, &rc );
	Height = rc.bottom - rc.top;
	Width = rc.right - rc.left;

	// define the array of driver types
	// required during swap chain creation
	D3D10_DRIVER_TYPE driverTypes[] =
    {
        D3D10_DRIVER_TYPE_HARDWARE,
        D3D10_DRIVER_TYPE_WARP,
        D3D10_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

	UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

	// //////////////////////////////////////
	// SWAPCHAIN
	//
	// create swap chain desc and set some info about swapchain
	DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 1;
    sd.BufferDesc.Width = Width;							// previously stored width
    sd.BufferDesc.Height = Height;							// previously stored height
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;		// standard rgb 8-bit format
    sd.BufferDesc.RefreshRate.Numerator = 60;				// standard 60 hz refresh rate
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;									// window handler passed as an argument
    sd.SampleDesc.Count = 1;								// no multisampling
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;										// stay windowed


	// create swap chain using the description already filled
	// search for the right driver starting from the best one
	// (if the best one succeeds, no need for further search)
	for( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        pDriverType = driverTypes[driverTypeIndex];
        hr = D3D10CreateDeviceAndSwapChain( NULL, pDriverType, NULL, createDeviceFlags,
                                            D3D10_SDK_VERSION, &sd, &pSwapChain, &pd3dDevice );
        if( SUCCEEDED( hr ) )
            break;
    }

	// ////////////////////////////////////////////
	// RENDER TARGET
	//
	// Create a render target view
    ID3D10Texture2D* pBuffer;
    hr = pSwapChain->GetBuffer( 0, __uuidof( ID3D10Texture2D ), ( LPVOID* )&pBuffer );
    if( FAILED( hr ) )
		ERRORMACRO( L"Texture initialization failed" );

    hr = pd3dDevice->CreateRenderTargetView( pBuffer, NULL, &pRenderTargetView );
    pBuffer->Release();
    if( FAILED( hr ) )
		ERRORMACRO( L"Render Target View initialization failed" );

	// ////////////////////////////////////////////
	// DEPTH STENCIL TEXTURE
	//
	// Create depth stencil texture
    D3D10_TEXTURE2D_DESC descDepth;
    descDepth.Width = Width;
    descDepth.Height = Height;
    descDepth.MipLevels = 1;
    descDepth.ArraySize = 1;
    descDepth.Format = DXGI_FORMAT_D32_FLOAT;
    descDepth.SampleDesc.Count = 1;
    descDepth.SampleDesc.Quality = 0;
    descDepth.Usage = D3D10_USAGE_DEFAULT;
    descDepth.BindFlags = D3D10_BIND_DEPTH_STENCIL;
    descDepth.CPUAccessFlags = 0;
    descDepth.MiscFlags = 0;

    hr = pd3dDevice->CreateTexture2D( &descDepth, NULL, &pDepthStencil );
    if( FAILED( hr ) )
		ERRORMACRO( L"DepthStencil texture initialization failed" );

    // Create the depth stencil view
    D3D10_DEPTH_STENCIL_VIEW_DESC descDSV;
    descDSV.Format = descDepth.Format;
    descDSV.ViewDimension = D3D10_DSV_DIMENSION_TEXTURE2D;
    descDSV.Texture2D.MipSlice = 0;

    hr = pd3dDevice->CreateDepthStencilView( pDepthStencil, &descDSV, &pDepthStencilView );
    if( FAILED( hr ) )
		ERRORMACRO( L"Depth Stencil View initialization failed" );	//*/

	// set render targets
	pd3dDevice->OMSetRenderTargets( 1, &pRenderTargetView, pDepthStencilView );

	 // Setup the viewport
    D3D10_VIEWPORT vp;
    vp.Width = Width;
    vp.Height = Height;
    vp.MinDepth = 0.0f;
    vp.MaxDepth = 1.0f;
    vp.TopLeftX = 0;
    vp.TopLeftY = 0;
    pd3dDevice->RSSetViewports( 1, &vp );

	// set primitive topology
	pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	return hr;
};

// paint scene method. as name suggests it paints the scene.
// needs Input, Camera and Space to work.
void	Mateyko::PaintScene()
{
	// ////////////////////////////////////
	// safety belt

	// if any of those devices isn't binded, or InitDevice
	// methods hasn't been called (it initializes the pd3dDevice)
	// don't do anything
	if( pd3dDevice == NULL )		return;
	if( pInput == NULL )			return;
	if( pCam == NULL )				return;
	if( pSpace == NULL )			return;

	// ////////////////////////////////////
    // Clear the back buffer

    float ClearColor[4] = { 0.0f, 0.4f, 0.9f, 1.0f }; // red, green, blue, alpha
    pd3dDevice->ClearRenderTargetView( pRenderTargetView, ClearColor );

	// /////////////////////////////////////
    // Clear the depth buffer to 1.0 (max depth)
    
    pd3dDevice->ClearDepthStencilView( pDepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );

	// ///////////////////////////////////
	// prepare general (same for all object on the scene) rendering parameters
	
	pInput->PrepareShadingControlVars(
		oColors.size() );
	
	pInput->PrepareCameraMatrices( 
		( float* )pCam->GetView().m,
		( float* )pCam->GetProjection().m );
	
	pInput->PrepareEyePos( 
		( float* )&pCam->GetEyePos() );
	
	pInput->PreparePositions( 
		pSpace->GetShaderPositionArray(), 
		pSpace->size() );
	
	pInput->PrepareColors( 
		( float* )oColors.data(),
		oColors.size() );
	
	// ////////////////////////////////////
	// Render objects on the scene
	for( unsigned int i = 0; i < objects.size(); i++ )	
	{
		// prepare object-oriented pInput variables
		pInput->PrepareObject( ( float* )pSpace->GetWorldPosition( i ).m, i );
		
		// DRAW!!!
		objects[ i ]->Draw( pd3dDevice, pInput->GetTech() );
	}
	
	// //////////////////////////////////////
	// render the floor
	
	// prepare object-oriented pInput variables for the floor
	if( oGroundZero )
	{
		pInput->PrepareObject( ( float* )XMMatrixTranslation( 0.0f, -1.0f, 0.0f ).m, -1 );
		oGroundZero->Draw( pd3dDevice, pInput->GetTech() );
	}

	// //////////////////////////////////////
    // Present our back buffer to our front buffer
    pSwapChain->Present( 0, 0 );
}

// method loads texture for the floor. it also erases previous texture if needed
HRESULT		Mateyko::loadTexture( LPCWSTR szFileName )
{
	HRESULT hr = S_OK;
	
	// remove previous texture if needed
	if( FloorTextureRV )	FloorTextureRV->Release();
	
	// bind new texture
	hr = D3DX10CreateShaderResourceViewFromFile( pd3dDevice, szFileName, NULL, NULL, &FloorTextureRV, NULL );
	if( FAILED( hr ) )
		ERRORMACRO( L"Cannot load texture." );
	
	// set resourece to the shader variable
	if( FloorTextureRV )	pInput->SetFloorTex( FloorTextureRV );
	return hr;
}

// saves stored client rectangle width and height to the provided references
void	Mateyko::GetClientRectSize( UINT& _width, UINT& _height )
{
	_width = Width;
	_height = Height;
}

// get device method
ID3D10Device*		Mateyko::GetDevice()				{ 	return pd3dDevice; 	}

// binding funcs. each of them bind respective device, and Mateyko won't draw anything without them
void	Mateyko::BindCamera( Camera* cam )				{	pCam = cam; 	}
void	Mateyko::BindSpace( Space* spa )				{	pSpace = spa; 	}

// bindinput also has to set input layout to the device
void	Mateyko::BindInput( ShaderInput* shi )			
{	
	pInput = shi; 
	pd3dDevice->IASetInputLayout( pInput->GetLayout() );	
}

// /////////////////////////////////////////////////////
// 
// MATEYKO OBJECT LIST MANAGEMENT METHODS
//
// /////////////////////////////////////////////////

// in case a pointer to the object was provided without
// color, copy the object and store predefined color
void	Mateyko::InsertObject( Object3D* o3d )
{
	// create the shared_ptr for o3d argument
	std::shared_ptr<Object3D>	o3ptr( new Object3D( *o3d ) );

	// copy object3d using copy constructor
	// construct shared_ptr using typical pointer
	objects.push_back( o3ptr );
	oColors.push_back( XMFLOAT4( 0.4f, 0.7f, 0.2f, 1.0f ) );
}

// creates the object using provided data then stores
// its pointer within objects vector and saves color
// to the oColors vector, ensuring every object
// has its corresponding color stored under the same
// index.
void	Mateyko::InsertObject( void* verts, DWORD* inds, UINT vSize, UINT iSize, XMFLOAT4 color )
{
	// create the shared_ptr for o3d argument
	std::shared_ptr<Object3D>	o3ptr( new Object3D( pd3dDevice, verts, inds, vSize, iSize ) );

	// do stuff
	objects.push_back( o3ptr );
	oColors.push_back( color );
}

// removes object and color with a corresponding index.
// causes a reallocation inside vectors, so better be
// used carefully. we do not expect much juggling with
// objects on the scene, therefore do not see the need
// for replacing vector with forward_list or map or sth.
void	Mateyko::RemoveObject( int oNum )
{
	objects.erase( objects.begin() + oNum );
	oColors.erase( oColors.begin() + oNum );
}

// removes all objects and their colors from the vectors
// leaving them empty.
void	Mateyko::RemoveAll()
{
	objects.clear();
	oColors.clear();
}

// sometimes its necesarry to updates objects color
// and thats what this method does.
void	Mateyko::updateColor( unsigned int oNumber, XMFLOAT4 color )
{
	if( oNumber < oColors.size() )
		oColors[ oNumber ] = color;
}

// /////////////////////////////////////////////////////
//
// MATEYKO FORM OBJECT METHODS
//
// /////////////////////////////////////////////////

// creates a sphere of desired radius and color and with
// desired number of meridians and parallels
// automatically generates both the set of the vertices (struct Vertex)
// and triangle list, then forms an Object3D using them
// finally stores the sphere into the
// objects std::vector of a Mateyko class
void Mateyko::formSphere( LPCWSTR _name, UINT meridians, UINT parallels, float radius, XMFLOAT4 color )
{
	// ////////////////////////////////////////////
	// DECLARE VARIABLES
	// ...
	// those two are the final vertices and final
	// indices that will be used to form an object
	std::vector< Vertex >	fnVertices;
	std::vector< DWORD >	fnIndices;

	// a temporary vertex struct which will be used
	// to push data back into the vector. color's always
	// the same, so it can be defined right now
	Vertex	vx;
	vx.Color = color;

	// to keep track of which vertex we are already painting
	// (that's necessary to find out its position and normal)
	// we employ xmvector brush. you can think of it 
	// travelling along the meridians and marking dots
	// (vertices) on the right spots. right spots are
	// computed using mAngle (meridian angle) and pAngle
	// (parallel angle) float variables
	XMVECTOR brush = XMVectorSet( 0.0f, radius, 0.0f, 0.0f );
	float mAngle, pAngle;
	
	// ///////////////////////////////////////////////
	// DEFINE VARIABLES
	// ...
	// define the angles
	mAngle = 2.0f * XM_PI / ( FLOAT )meridians;
	pAngle = XM_PI / ( FLOAT )parallels;

	// set the first point. it's the "top" point of the sphere
	// that belongs to every parallel (and starts it in fact)
	XMStoreFloat3( &vx.Pos, brush );
	XMStoreFloat3( &vx.Norm, XMVector3Normalize( brush ) );
	fnVertices.push_back( vx );

	// and set the "last" point. it ends every parallel
	XMStoreFloat3( &vx.Pos, -brush );
	XMStoreFloat3( &vx.Norm, XMVector3Normalize( -brush ) );
	fnVertices.push_back( vx );

	// ///////////////////////////////////////////
	// SET THE VERTICES VECTOR
	// ...
	// the outer loop runs through all the declared (in meridians
	// int variable) meridians, adding respective number
	// to the mAngle factor, ensuring each meridian created
	// by one iteration is retorsed on the xz plane by
	// the right angle
	for( unsigned int i = 0; i < meridians; i++ )
	{
		// each iteration adds a vertex to the current meridian
		// and pushes the brush by the mAngle
		for( unsigned int j = 0; j < parallels - 1; j++ )
		{
			// rotate brush around x axis for this cycle
			// as for the first (j=0) iteration the brush was set up straight
			// so it start with the second vertex of a first (on the xz plane)
			// meridian, and it will be later rotated around y axis.
			// every iteration then it adds pAngle until the 
			// one before the last occurs.
			brush = XMVector3TransformCoord( brush, XMMatrixRotationX( pAngle ) );

			// set the point. first, rotate around y axis, then store position
			// and normal. normal comes from the normalized difference between 
			// distance and the 0 point, since the sphere's centre is that point
			XMStoreFloat3( &vx.Pos, XMVector3TransformCoord( brush, XMMatrixRotationY( mAngle * i ) ) );
			XMStoreFloat3( &vx.Norm, XMVector3Normalize( XMVector3TransformCoord( 
				brush, XMMatrixRotationY( mAngle * i ) ) ) );

			// finally push it into the vertices
			fnVertices.push_back( vx );
		}

		// set the brush up straight for the next cycle
		// the top vertex is already set, but the loop above
		// increases the pAngle before anything else
		// so it will start with the right vertex
		brush = XMVectorSet( 0.0f, radius, 0.0f, 0.0f );
	}

	// ///////////////////////////////////////////////
	// SET THE INDICES VECTOR
	// ...
	// same as in the loop above, each iteration
	// constructs the grid between two meridians
	// the construction of the grid between the last
	// and the first meridian is done separetely below
	// for the sake of the simplicity of the code
	for( unsigned int i = 0; i < meridians - 1; i++ )
	{
		// set the first triangle. this is only one triagle
		// (not the square composed of two triangles)
		// for both meridians start in the same point
		fnIndices.push_back( 0 );
		fnIndices.push_back( ( i+1 ) * ( parallels - 1 ) + 2 );
		fnIndices.push_back( ( i ) * ( parallels - 1 ) + 2 );	

		// midst triangles. each iteration of this loop
		// handles one square set up from two pairs of points
		// of the same parallel from two meridians.
		for( unsigned int j = 0; j < parallels - 2; j++ )
		{
			// first triangle
			fnIndices.push_back( i * ( parallels - 1 ) + 2 + j );
			fnIndices.push_back( ( i+1 ) *( parallels - 1 ) + 2 + j );
			fnIndices.push_back( ( i+1 ) *( parallels - 1 ) + 2 + j + 1 );	

			// second triangle
			fnIndices.push_back( i * ( parallels - 1 ) + 2 + j );
			fnIndices.push_back( ( i+1 ) *( parallels - 1 ) + 2 + j + 1 );
			fnIndices.push_back( i * ( parallels - 1 ) + 2 + j + 1 );
		}

		// last triangle. both meridians end up with
		// the same vertex, so only one triangle here
		fnIndices.push_back( 1 );
		fnIndices.push_back( ( i ) * ( parallels - 1 ) -1 + parallels );
		fnIndices.push_back( ( i+1 ) * ( parallels - 1 ) -1 + parallels );	
	}

	// ///////////////////////////////////////////////
	// THE LAST MERIDIAN
	// ...
	// everything goas as within the loop
	// so there's nothing much to explain
	int i = meridians - 1;
	for( unsigned int j = 0; j < parallels - 2; j++ )
	{
		// first triangle
		fnIndices.push_back( i * ( parallels - 1 ) + 2 + j );
		fnIndices.push_back( 2 + j );
		fnIndices.push_back( 2 + j + 1 );	

		// second triangle
		fnIndices.push_back( i * ( parallels - 1 ) + 2 + j );
		fnIndices.push_back( 2 + j + 1 );
		fnIndices.push_back( i * ( parallels - 1 ) + 2 + j + 1 ); 
	}

	// first triangle 
	fnIndices.push_back( 0 );
	fnIndices.push_back( 2 );
	fnIndices.push_back( ( i ) * ( parallels - 1 ) + 2 );

	// last triangle
	fnIndices.push_back( 1 );
	fnIndices.push_back( i * ( parallels - 1 ) -1 + parallels );
	fnIndices.push_back( -1 + parallels );	

	// //////////////////////////////////////
	// indices swap - if rasters set back

	// swaps the order of the triangles in the indices set
	// basicly turns them from counter-clockwise into clockwise
	for( unsigned int i = 0; i < fnIndices.size() / 3; i++ )
	{
		std::swap( fnIndices.at( i*3 ), fnIndices.at( i*3 +2 ) );
	}

	// //////////////////////////////////////
	// final func stage

	InsertObject( fnVertices.data(), fnIndices.data(), fnVertices.size(), fnIndices.size(), color );
}

// creates a rectangle surface of desired length and width.
// surface' normal is described by planeNormal variable
// and lenDir is the direction of the surface (egdes
// parallel to that direction are described by length var)
void	Mateyko::formRectangleObject( LPCWSTR _name, float length, float width, XMFLOAT3 planeNormal, XMFLOAT3 lenDir )
{
	// ////////////////////////////////////////
	// DECLARE VARIABLES
	// ...
	// those two are the final vertices and final
	// indices that will be used to form an object
	std::vector< Vertex >	fnVertices;
	std::vector< DWORD >	fnIndices;
	
	XMMATRIX	mxRectSpace;	// transformation matrix
	Vertex		tmpVertex;		// we store here the vertex we want push back into vector
	
	// combining those floats and their negatives in the
	// space created by planeNormal and lenDir gives us
	// all four vertices that form the rectangle
	float xC = length * 0.5f;
	float zC = width * 0.5f;

	// aquire the base of the rectangle space.
	// in this space previously defined xC and zC
	// are enough to get the vertex of the rectangle
	mxRectSpace = getSpaceMatrix( lenDir, planeNormal, true );

	// prepare tmpVertex
	tmpVertex.Color = XMFLOAT4( 0.8f, 0.1f, 0.3f, 1.0f );
	tmpVertex.Norm = planeNormal;
	
	// calculate positions and store vertices - 1st
	XMStoreFloat3( &tmpVertex.Pos, XMVector3TransformCoord( XMVectorSet( -xC, 0.0f, -zC, 0.0f ), mxRectSpace ) );
	fnVertices.push_back( tmpVertex );

	// 2nd
	XMStoreFloat3( &tmpVertex.Pos, XMVector3TransformCoord( XMVectorSet( -xC, 0.0f, zC, 0.0f ), mxRectSpace ) );
	fnVertices.push_back( tmpVertex );

	// 3rd
	XMStoreFloat3( &tmpVertex.Pos, XMVector3TransformCoord( XMVectorSet( xC, 0.0f, zC, 0.0f ), mxRectSpace ) );
	fnVertices.push_back( tmpVertex );

	// 4th
	XMStoreFloat3( &tmpVertex.Pos, XMVector3TransformCoord( XMVectorSet( xC, 0.0f, -zC, 0.0f ), mxRectSpace ) );
	fnVertices.push_back( tmpVertex );

	// get indices set
	fnIndices.push_back( 0 );
	fnIndices.push_back( 1 );
	fnIndices.push_back( 2 );
	fnIndices.push_back( 0 );
	fnIndices.push_back( 2 );
	fnIndices.push_back( 3 );

	// //////////////////////////////////////
	// final func stage
	
	// note: now this raytracer don't need
	// any other rectangles apart from its
	// ground. so instead of returning the
	// vertices and indices create the obj
	// and attach it to GroundZero pointer

	// in case ground was already created remove preous one
	if( oGroundZero )
		delete oGroundZero;
		
	// create new ground object
	oGroundZero = new Object3D( 
		pd3dDevice, 
		fnVertices.data(), 
		fnIndices.data(), 
		fnVertices.size(), 
		fnIndices.size() );
}

// ////////////////////////////////////////////////
// ///////////////////////////////////////////////
// //////////////////////////////////////////////
// 
// SHADER INPUT	:	METHODS, CONSTRUCTORS AND OPERATORS DEFINITIONS
// 
// /////////////////////////////////////////
// ////////////////////////////////////////
// ///////////////////////////////////////

// main constructor of the ShaderInput class
// only one since the copy & default were disabled.
// creates shaders from file and binds them to 
// Effect variable. obtain technique from that file
// and saves it to the Technique variable. then creates 
// input layout using static vertex_desc variable.
// in the end initializes all shader variables 
// using previously creted Effect.

ShaderInput::ShaderInput( 
	ID3D10Device*	pd3dDevice,		// pointer to the device that will create the input
	LPCWSTR szFileName, 			// path to a file contains shaders
	LPCSTR szTechName )				// name of the technique defined in the shader file
{
	// variables
	D3D10_PASS_DESC PassDesc;
	HRESULT hr = S_OK;
	UINT numElem = sizeof( vertex_desc ) / sizeof( vertex_desc[0] );
	DWORD dwShaderFlags = D3D10_SHADER_ENABLE_STRICTNESS;

	// create EFFECT
	hr = D3DX10CreateEffectFromFile( 
		szFileName, 
		NULL, NULL, "fx_4_0", 
		dwShaderFlags, 0, 
		pd3dDevice, 
		NULL, NULL, 
		&Effect, 
		NULL, NULL );

	if( FAILED( hr ) )
		ERRORMACRO( L"Critical failure during shader compilation process." );

	// create TECHNIQUE
	Technique = Effect->GetTechniqueByName( szTechName );

	// Create the input layout using the first tech, var INPUT
	Technique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
    hr = pd3dDevice->CreateInputLayout( 
		vertex_desc, 
		numElem, 
		PassDesc.pIAInputSignature,
        PassDesc.IAInputSignatureSize, 
		&Input );
	
	if( FAILED( hr ) )
		ERRORMACRO( L"Nie jest fajno, szefie" );
		
	// when the most important variables are being handled
	// its time to define variables that will pass data to
	// the constant buffers.

	// create effect matrix variables
	World 		= Effect->GetVariableByName( 	"World" 		)->AsMatrix();
	View 		= Effect->GetVariableByName( 	"View" 			)->AsMatrix();
	Projection 	= Effect->GetVariableByName( 	"Projection" 	)->AsMatrix();

	// create effect vector variables
	Light 		= Effect->GetVariableByName( 	"Light" 		)->AsVector();
	CamEye 		= Effect->GetVariableByName( 	"CamEye" 		)->AsVector();
	BigBalls 	= Effect->GetVariableByName( 	"BigBalls" 		)->AsVector();
	OColors 	= Effect->GetVariableByName( 	"OColors" 		)->AsVector();

	// create effect scalar variables
	// look to the class header for further descriptions
	num_processed 		= Effect->GetVariableByName( 	"num_processed" 	)->AsScalar();
	count_processed 	= Effect->GetVariableByName( 	"count_processed" 	)->AsScalar();
	brightness 			= Effect->GetVariableByName( 	"brightness" 		)->AsScalar();
	reflectance 		= Effect->GetVariableByName( 	"reflectance" 		)->AsScalar();
	skybright 			= Effect->GetVariableByName( 	"skybright" 		)->AsScalar();
	diffuseStr 			= Effect->GetVariableByName( 	"diffuseStr" 		)->AsScalar();
	gamma 				= Effect->GetVariableByName( 	"gamma" 			)->AsScalar();
	ColorNumVar 		= Effect->GetVariableByName( 	"ColorNumVar" 		)->AsScalar();

	// create texture and map variables
	FloorTexture = Effect->GetVariableByName( "FloorTexture" )->AsShaderResource();

	// prepare values that need to be passed to shaders
	vGamma = 2.2f;	
	vBrightness = 0.8f;	
	vReflectance = 2.35f;
	vSkyBrightness = 1.1f;	
	vDiffusePower = 1.25f;	
	vChannel = 0;			
}

// ShaderInput destructor.
// destructs things pretty nicely.
ShaderInput::~ShaderInput()
{
	// release things
	if( Input )			Input->Release();
	if( Effect )		Effect->Release();
	
	// according to the msdn all effect variables
	// are released when its parent effect is released.
	// so we do not do it manually.
}

// passes almost all shading control variables
// (except the num_processed which is diffrent
// for every object on the scene) to the shader's
// constants buffers. used in PaintScene
void	ShaderInput::PrepareShadingControlVars( int arg )
{
	ColorNumVar->SetInt( 		vChannel );
	brightness->SetFloat( 		vBrightness );
	reflectance->SetFloat( 		vReflectance );
	gamma->SetFloat( 			vGamma );
	diffuseStr->SetFloat( 		vDiffusePower );
	skybright->SetFloat( 		vSkyBrightness );
	count_processed->SetInt(	arg );
}

// passes the arguments to the shaders
// those arguments are view and projection
// matrices, generated by a Camera object,
// passed as a pointers to the matrix' float array
void	ShaderInput::PrepareCameraMatrices( 
	float* _view, 
	float* _proj )
{
	View->SetMatrix( _view );
	Projection->SetMatrix( _proj );
}

// passes the argument (which is an xmfloat
// camera's eye position converted to the float*)
// to the shaders.
void	ShaderInput::PrepareEyePos( float* _eye )
{
	CamEye->SetFloatVector( _eye );
}

// passes positions of all objects and the
// number of objects to the shaders. positions
// are, as usual, provided as an array of floats
void	ShaderInput::PreparePositions( 
	float* _pos, 
	int _count )
{
	BigBalls->SetFloatVectorArray( _pos, 0, _count );
}

// passes colors the same way did with positions
void	ShaderInput::PrepareColors( 
	float* _colors, 
	int _count )
{
	OColors->SetFloatVectorArray( _colors, 0, _count );
}

// passes current object's world matrix 
// and its index on the object list. 
void	ShaderInput::PrepareObject( 
	float* _matrix, 
	int _index )
{
	num_processed->SetInt( _index );
	World->SetMatrix( _matrix );
}
	
void	ShaderInput::SetFPS( float arg )								{ 	fps = arg; 	}
void	ShaderInput::SetFloorTex( ID3D10ShaderResourceView* shevi )		{	FloorTexture->SetResource( shevi ); 	}

// depending on which variable is pointed by
// the varIndex, method adds or subtracts 
// to/from that variable.
void	ShaderInput::NmpdAddSubtract( double arg )
{
	// negOrPos is short for negative or positive.
	// it basically tells us if the passed argument
	// (double arg) is above or below or equal to zero
	// multiplying by sigma reduce the necessity for
	// creating two branches in an alghoritm
	// when we want to add or subtract to a paricular
	// variable depending on sign of the add argument
	float	negOrPos;
	if( arg >= 0.0f )	
		negOrPos = 1.0f;
	else negOrPos = -1.0f;
	
	// decide which shading control var is to be
	// incremented or decremented depending on 
	// varIndex variable (which is set by NmpdNumber method)
	switch( varIndex )
	{
	
	// floating point vars
	case 1:		vGamma 				+= negOrPos * 0.1f / fps;	break;
	case 2:		vBrightness 		+= negOrPos * 0.2f / fps;	break;
	case 3:		vReflectance 		+= negOrPos * 0.4f / fps;	break;
	case 4:		vDiffusePower 		+= negOrPos * 0.4f / fps;	break;
	case 5:		vSkyBrightness 		+= negOrPos * 0.4f / fps;	break;
	
	// integer var. we want to set limits to it, so user won't get far
	// from those few channels out there.
	case 6:
		if( vChannel <= 14 && arg >= 0.0f )		vChannel++;
		else if( vChannel > 0 && arg < 0.0f )	vChannel--;
		break;
	}
}

// simply sets the index for the NmpdAddSubtract method.
void	ShaderInput::NmpdNumber( int arg )	{	varIndex = arg; 	}

ID3D10EffectTechnique* const		ShaderInput::GetTech()		{	return Technique;	}
ID3D10InputLayout* const			ShaderInput::GetLayout()	{ 	return Input; 		}

// ////////////////////////////////////////////////
// ///////////////////////////////////////////////
// //////////////////////////////////////////////
// 
// CAMERA	:	METHODS, CONSTRUCTORS AND OPERATORS DEFINITIONS
// 
// /////////////////////////////////////////
// ////////////////////////////////////////
// ///////////////////////////////////////

// default constructor
Camera::Camera()
	:	Eye( 10.0f, 5.0f, 0.0f ),
		At( 0.0f, 0.0f, 0.0f ),
		Up( 0.0f, 1.0f, 0.0f ),
		// starting direction
		
		FoV( 1.0f ),
		fps( 1.0f ),
		ScreenRatio( 1.0f ),
		// those three need to be regulated separately via
		// setters. set for 1.0f to avoid any problems with division
		
		acceleration( 3.0f ),
		braking( -3.0f ),		
		// needs to be negative, because we're adding it to the velos
		// both acc.. and br.. have fixed values for now
		
		veloUpDown( 0.0f ),
		veloLeftRight( 0.0f ),
		veloEyeRot( 0.0f ),
		veloAtRot( 0.0f )
		// velocities starts at 0.0f
		// as they increase when user
		// presses the buttons or moves
		// the mouse.
{}

// the second constructor
// look to the default one for explanations
Camera::Camera( XMFLOAT3 _eye, XMFLOAT3 _at, XMFLOAT3 _up )
	:	Eye( _eye ),
		At( _at ),
		Up( _up ),
		
		FoV( 1.0f ),
		fps( 1.0f ),
		ScreenRatio( 1.0f ),
		
		acceleration( 3.0f ),
		braking( -3.0f ),		
		
		veloUpDown( 0.0f ),
		veloLeftRight( 0.0f ),
		veloEyeRot( 0.0f ),
		veloAtRot( 0.0f )
{}

void	Camera::SetFPS( float arg )								{ 	fps = arg; 	}
void	Camera::SetScreenRatio( float arg )						{	ScreenRatio = arg;	}
void	Camera::SetFoV( float arg )								{	FoV = arg; 	}

void	Camera::UpdateCam()
{
	// first, reduce the velocities
	veloUpDown 		+= braking / fps;
	veloLeftRight 	+= braking / fps;
	veloEyeRot		+= braking / fps;
	veloAtRot		+= braking / fps;
	
	// then check if any velocity is lower that zero and handle that
	if( veloUpDown < 0.0f ) 	veloUpDown = 0.0f;
	if( veloLeftRight < 0.0f ) 	veloLeftRight = 0.0f;
	if( veloEyeRot < 0.0f ) 	veloEyeRot = 0.0f;
	if( veloAtRot < 0.0f ) 		veloAtRot = 0.0f;
}

void	Camera::ArrowsUpDown( double arg )
{
	// DECLARE VARIABLES
	XMVECTOR	vtrAt, vtrEye;		// xmvector equivalents of Eye and At xmfloats
	XMVECTOR	movDir;				// the direction on the xz plane in which camera looks
	float	negOrPos;

	// DEFINE VARIABLES
	vtrAt = XMLoadFloat3( &At );
	vtrEye = XMLoadFloat3( &Eye );
	
	// we first get the difference vector between
	// the eye and at points, then remove y factor
	// so we have direction parallel to the xz plane
	// then normalize it, so the distance between
	// at and eye does not affect the speed increment

	if( arg >= 0.0f )	
		negOrPos = 1.0f;
	else negOrPos = -1.0f;
	
	movDir = vtrAt - vtrEye;
	movDir = XMVectorSetY( movDir, 0.0f );
	movDir = XMVector3Normalize( movDir );
	
	// if velocity didn't reach its max level increase
	if( veloUpDown < 1.0f ) 
		veloUpDown += acceleration / fps;
	
	// we divide by fps value, because we don't want
	// refresh rate to affect the speed
	vtrAt += negOrPos * veloUpDown * movDir / fps;
	vtrEye += negOrPos * veloUpDown * movDir / fps;
	
	// store incremented values
	XMStoreFloat3( &At, vtrAt );
	XMStoreFloat3( &Eye, vtrEye );
}

void	Camera::ArrowsLeftRight( double arg )
{	
	// DECLARE VARIABLES
	XMVECTOR	vtrAt, vtrEye;		// xmvector equivalents of Eye and At xmfloats
	XMVECTOR	movDir;				// the direction on the xz plane in which camera looks
	
	// DEFINE VARIABLES
	vtrAt = XMLoadFloat3( &At );
	vtrEye = XMLoadFloat3( &Eye );
	
	// the first line of code below is similar to respective line 
	// in the ArrowsUpDown method, look there for explanation.
	// next we take the cross product which is perpendicular
	// to the direction camera is facing, and that means 
	// its pointing right. if needed we multiply it by
	// -1.0f so it points left.
	
	movDir = XMVector3Normalize( XMVectorSetY( ( vtrAt - vtrEye ), 0.0f ) );
	movDir = XMVector3Cross( movDir, XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f ) );
	if( arg < 0.0f )
		movDir = movDir * -1.0f;
		
	// if velocity didn't reach its max level increase
	if( veloLeftRight < 1.0f ) 
		veloLeftRight += acceleration / fps;
		
	// we divide by fps value, because we don't want
	// refresh rate to affect the speed
	vtrAt += veloLeftRight * movDir / fps;
	vtrEye += veloLeftRight * movDir / fps;
	
	// store incremented values
	XMStoreFloat3( &At, vtrAt );
	XMStoreFloat3( &Eye, vtrEye );
}

// this methods rotates the eye around at point. to do so
// we want to find vector that is perpendicular both to 
// the zx plane and the vertical plane along which we 
// want to move the eye point. such vector is required 
// for the rotation matrix construction.
// arg says how far we want to rotate (in radians)
void	Camera::MouseUpDown( double arg )
{
	// DECLARE VARIABLES
	XMMATRIX	mxRota;				// matrix that will help us get the final eye position
	XMVECTOR	vtrAt, vtrEye;		// xmvector equivalents of Eye and At xmfloats
	XMVECTOR	movDir;				// the direction on the xz plane in which camerais looking
	float		atEyeDist;
	float		cosine;
	
	// DEFINE VARIABLES
	vtrAt = XMLoadFloat3( &At );
	vtrEye = XMLoadFloat3( &Eye );
	
	// we need to check if rotation around
	// the at point won't left camera upside down.
	// technically i wouldn't be as much of a problem
	// since the class generates the view matrix
	// every time anew, but it might confuse the user
	
	movDir = vtrEye - vtrAt;
	cosine = XMVectorGetX( XMVector3Dot( XMVector3Normalize( movDir ), XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f ) ) );
	if( ( cosine > 0.9f && arg >= 0.0f ) ||
		( cosine < 0.9f && arg < 0.0f ) )
		return;
	
	// store the distance between at and eye position
	atEyeDist = XMVectorGetX( XMVector3Length( movDir ) );
	
	// operation below is explained in the respective
	// comment section in the methods above this one.
	movDir = XMVector3Normalize( XMVectorSetY( movDir, 0.0f ) );
	movDir = XMVector3Cross( movDir, XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f ) );
	
	// rotate the at-eye vector around an at point,
	// making it pointing the new eye position
	mxRota = XMMatrixRotationAxis( movDir, arg );	
	movDir = XMVector3TransformCoord( movDir, mxRota );
	
	// find new eye position
	vtrEye = vtrAt + atEyeDist * movDir;
	
	// store final result
	XMStoreFloat3( &Eye, vtrEye );
}

// this method is similar to the previous one, only
// instead of vertical rotation we employ horizontal
// (to the viewer) rotation around at point. such
// similarity allows us to use the same solutions
// as in the method above. we too construct rotation
// matrix, but this time around 0-1-0 vector.
void	Camera::MouseLeftRight( double arg )
{
	// DECLARE VARIABLES
	XMMATRIX	mxRota;				// matrix that will help us get the final eye position
	XMVECTOR	vtrAt, vtrEye;		// xmvector equivalents of Eye and At xmfloats
	XMVECTOR	movDir;				// the direction on the xz plane in which camera looks
	
	// DEFINE VARIABLES
	vtrAt = XMLoadFloat3( &At );
	vtrEye = XMLoadFloat3( &Eye );
	
	// construct the rotation matrix
	// and rotate the at-eye vector
	mxRota = XMMatrixRotationAxis( XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f ), arg );
	movDir = vtrEye - vtrAt;
	movDir = XMVector3TransformCoord( movDir, mxRota );
	
	// find new eye position
	vtrEye = vtrAt + movDir;
	
	// store final result
	XMStoreFloat3( &Eye, vtrEye );
}

// wsadleftright works same as mouseleftright
// except the at point is rotated around eye.
// so, if want any explanations, look to
// the mouseleftright.
void	Camera::WsadLeftRight( double arg )
{
	// DECLARE VARIABLES
	XMMATRIX	mxRota;				// matrix that will help us get the final eye position
	XMVECTOR	vtrAt, vtrEye;		// xmvector equivalents of Eye and At xmfloats
	XMVECTOR	movDir;				// the direction on the xz plane in which camera looks
	
	// DEFINE VARIABLES
	vtrAt = XMLoadFloat3( &At );
	vtrEye = XMLoadFloat3( &Eye );
	
	// rotate the movDir
	mxRota = XMMatrixRotationAxis( XMVectorSet( 0.0f, 1.0f, 0.0f, 0.0f ), XM_PI * veloEyeRot / fps );
	movDir = vtrAt - vtrEye;
	movDir = XMVector3TransformCoord( movDir, mxRota );
	
	// find new eye position
	vtrAt = vtrEye + movDir;
	
	// store final result
	XMStoreFloat3( &At, vtrAt );
}

// returns view matrix
XMMATRIX	Camera::GetView()
{
	return XMMatrixLookAtLH( 
		XMLoadFloat3( &Eye ), 		// eye or camera position. point where view frustum begins
		XMLoadFloat3( &At ), 		// point which camera is focusing on
		XMLoadFloat3( &Up ) );		// world's up direction
}

// returns projection matrix
XMMATRIX	Camera::GetProjection()
{
	return XMMatrixPerspectiveFovLH( 
		FoV, 						// field of view in radians
		ScreenRatio, 				// ratio between the width and height of a client rectancgle
		0.1f, 						// distance to the near clipping plane
		100.0f );					// distance to the far clipping plane
}

// returns eye position. 
XMFLOAT4	Camera::GetEyePos()
{
	return XMFLOAT4( Eye.x, Eye.y, Eye.z, 0.0f );
}

// ////////////////////////////////////////////////
// ///////////////////////////////////////////////
// //////////////////////////////////////////////
// 
// OBJECT3D	:	METHODS, CONSTRUCTORS AND OPERATORS DEFINITIONS
// 
// /////////////////////////////////////////
// ////////////////////////////////////////
// ///////////////////////////////////////

// main constructor of the Object3D class
// (default was disabled)
Object3D::Object3D( 
	ID3D10Device* pd3dDevice, 	// pointer to the device that will create buffers
	void* vertices, 			// pointer to the array of Vertex structure in which we store the grid
	DWORD* indices, 			// indices defining the triangles of that grid
	UINT _vSize, 				// size of both arrays
	UINT _iSize )
	
	:	vSize( _vSize ), iSize( _iSize ),
		stride( sizeof( Vertex ) ), offset( 0 )
{
	// declare variables
	HRESULT hr = S_OK;
	D3D10_BUFFER_DESC bd;
	ZeroMemory( &bd, sizeof( bd ) );

	// prepare vertex buffer
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( Vertex ) * vSize;
	bd.BindFlags = D3D10_BIND_VERTEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;

	D3D10_SUBRESOURCE_DATA InitData;
	InitData.pSysMem = vertices;

	// store the actual buffer pointer and run CreateDevice method
	hr = pd3dDevice->CreateBuffer( &bd, &InitData, &vBuffer );
	if( FAILED( hr ) )
		ERRORMACRO( L"Object construction failed. Unable to create vertex buffer" );

	// prepare index buffer
	bd.Usage = D3D10_USAGE_DEFAULT;
	bd.ByteWidth = sizeof( DWORD ) * iSize;
	bd.BindFlags = D3D10_BIND_INDEX_BUFFER;
	bd.CPUAccessFlags = 0;
	bd.MiscFlags = 0;
	InitData.pSysMem = indices;

	// store the actual buffer pointer and run CreateDevice method
	hr = pd3dDevice->CreateBuffer( &bd, &InitData, &iBuffer );
	if( FAILED( hr ) )
		ERRORMACRO( L"Object construction failed. Unable to create index buffer" );
}

// copy constructor
// copies the stuff that can be copied,
// then copies buffers pointers and increase
// their uses count
Object3D::Object3D( const Object3D& o3d )
	:	vSize( o3d.vSize ), 
		iSize( o3d.iSize ),
		stride( o3d.stride ), 
		offset( o3d.offset )
{
	// assign the buffers
	vBuffer = o3d.vBuffer;
	iBuffer = o3d.iBuffer;
	
	// increment the buffers uses count
	o3d.vBuffer->AddRef();
	o3d.iBuffer->AddRef();
}

// assigment operator. works similar to the 
// copy constructor. assign buffers pointers
// then increases uses count.
Object3D&	Object3D::operator=( const Object3D& o3d )
{
	if( this != &o3d )
	{
		// release the left operand's buffers
		vBuffer->Release();
		iBuffer->Release();
		
		// assign everything
		vSize = o3d.vSize; 
		iSize = o3d.iSize;
		stride = o3d.stride; 
		offset = o3d.offset;
		
		// assign the buffers
		vBuffer = o3d.vBuffer;
		iBuffer = o3d.iBuffer;
		
		// increment the buffers uses count
		o3d.vBuffer->AddRef();
		o3d.iBuffer->AddRef();

		return *this;
	}
}

// destructor. 
Object3D::~Object3D()
{
	vBuffer->Release();
	iBuffer->Release();
}

// function draws the object on the scene using provided device
void	Object3D::Draw( 
	ID3D10Device* pd3dDevice, 		// pointer to the device that will do the actual work
	ID3D10EffectTechnique* Tech )	// pointer to the technique that needs to be used
{
	// Variables
	D3D10_TECHNIQUE_DESC techDesc;

	// get vertex buffer and pass it into a device
	pd3dDevice->IASetVertexBuffers( 0, 1, &vBuffer, &stride, &offset );

	// set index buffer
	pd3dDevice->IASetIndexBuffer( iBuffer, DXGI_FORMAT_R32_UINT, 0 );
	
	Tech->GetDesc( &techDesc );	
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		Tech->GetPassByIndex( p )->Apply( 0 );
		pd3dDevice->DrawIndexed( iSize, 0, 0 );
	}
}

// ///////////////////////////////////////////////
// //////////////////////////////////////////////
// 
// REMAINING PROCEDURES DEFINITIONS
// 
// /////////////////////////////////////////

// given two vectors and a third vector direction, function 
// creates a matrix describing a 3-dimensional carthesian
// space with the xVec and yVec being its x and y axes
XMMATRIX	getSpaceMatrix( XMFLOAT3 xVec, XMFLOAT3 yVec, bool dirZ )
{
	XMVECTOR	bongo, xDir, yDir, zDir;
	xDir = XMLoadFloat3( &xVec );
	yDir = XMLoadFloat3( &yVec );
	if( dirZ )	
		zDir = XMVector3Cross( xDir, yDir );
	else zDir = XMVector3Cross( yDir, xDir );
	return XMMatrixInverse( &bongo, 
		XMMATRIX( xDir, yDir, zDir, XMVectorSet( 0.0f, 0.0f, 0.0f, 1.0f ) ) );
}