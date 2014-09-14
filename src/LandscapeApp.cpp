#include "cinder/app/AppBasic.h"
#include "cinder/gl/gl.h"
#include "cinder/Camera.h"
#include "cinder/MayaCamUI.h"
#include "cinder/gl/GlslProg.h"
#include "cinder/gl/Texture.h"
#include "cinder/ImageIo.h"
#include "cinder/ObjLoader.h"
#include "cinder/params/Params.h"
#include "cinder/Rand.h"
#include "cinder/Text.h"
#include "cinder/Font.h"

#include "boost/function.hpp"
#include "boost/bind.hpp"
#include "boost/lambda/lambda.hpp"
#include "boost/python.hpp"

#include "DeferredRenderer.h"
#include "Resources.h"

#include <chrono>

using namespace ci;
using namespace ci::app;
using namespace std;


static const float	APP_RES_HORIZONTAL = 1024.0f;
static const float	APP_RES_VERTICAL = 768.0f;
static const Vec3f	CAM_POSITION_INIT( -14.0f, 7.0f, -14.0f );
static const Vec3f	LIGHT_POSITION_INIT( 3.0f, 1.5f, 0.0f );

struct AzmAlt {
  double azm;
  double alt;
};

class LandscapeApp : public AppBasic
{
  public:
    void prepareSettings( Settings *settings );
    void setup();
    void update();
    void draw();
    void shutdown();
  
    void keyDown( app::KeyEvent event );
    void mouseDown( MouseEvent event );
    void mouseDrag( MouseEvent event );
    
    void drawShadowCasters(gl::GlslProg* deferShader) const;
    void drawNonShadowCasters(gl::GlslProg* deferShader) const;
    void drawOverlay() const;
  
  
  protected:
    //debug
    cinder::params::InterfaceGl mParams;
    int RENDER_MODE;
    bool mShowParams;
    float	mCurrFramerate;
	
    //camera
    MayaCamUI mMayaCam;
    CameraPersp mCam;
  
    // renderer
    DeferredRenderer mDeferredRenderer;
    int mCurrLightIndex;
  
    // object loading
    TriMesh mMesh;
    gl::VboMesh mVBO;
  
    // light positions
    AzmAlt getSunPosition(double hours);
    AzmAlt getMoonPosition(double hours);
  
    // python
    PyObject* py_ephem;
    PyObject* py_em;
  
  
  
    // time
    double mTimeHours;
    double getNowHours();

    AzmAlt mSunPos;
    AzmAlt mMoonPos;
  
    int count;
};



void LandscapeApp::prepareSettings( Settings *settings )
{
	settings->setWindowSize( APP_RES_HORIZONTAL, APP_RES_VERTICAL );
  settings->setBorderless( false );
	settings->setFrameRate( 1000.0f );			//the more the merrier!
	settings->setResizable( false );			//this isn't going to be resizable
  settings->setFullScreen( false );
    
	//make sure secondary screen isn't blacked out as well when in fullscreen mode ( do wish it could accept keyboard focus though :(
	//settings->enableSecondaryDisplayBlanking( false );
}

void LandscapeApp::setup()
{
    //!!test texture for diffuse texture
  
	gl::disableVerticalSync(); //so I can get a true representation of FPS (if higher than 60 anyhow :/)
    
	RENDER_MODE = DeferredRenderer::SHOW_FINAL_VIEW;
    
	mParams = params::InterfaceGl( "3D_Scene_Base", Vec2i( 225, 125 ) );
	mParams.addParam( "Framerate", &mCurrFramerate, "", true );
    mParams.addParam( "Selected Light Index", &mCurrLightIndex);
	mParams.addParam( "Show/Hide Params", &mShowParams, "key=x");
	mParams.addSeparator();
  
  // load object
  ObjLoader loader( (DataSourceRef)loadResource( RES_LANDSCAPE_OBJ ) );
  loader.load( &mMesh );
  mVBO = gl::VboMesh( mMesh );
  
    
	mCurrFramerate = 0.0f;
	mShowParams = true;
	
	//set up camera
	mCam.setPerspective( 45.0f, getWindowAspectRatio(), 0.1f, 10000.0f );
  mCam.lookAt(CAM_POSITION_INIT * 1.5f, Vec3f::zero(), Vec3f(0.0f, 1.0f, 0.0f) );
  mCam.setCenterOfInterestPoint(Vec3f::zero());
  mMayaCam.setCurrentCam(mCam);
  
  //create functions pointers to send to deferred renderer
  boost::function<void(gl::GlslProg*)> fRenderShadowCastersFunc = boost::bind( &LandscapeApp::drawShadowCasters, this, boost::lambda::_1 );
  boost::function<void(gl::GlslProg*)> fRenderNotShadowCastersFunc = boost::bind( &LandscapeApp::drawNonShadowCasters, this,  boost::lambda::_1 );
  boost::function<void(void)> fRenderOverlayFunc = boost::bind( &LandscapeApp::drawOverlay, this );
  
  //NULL value represents the opportunity to a function pointer to an "overlay" method. Basically only basic textures can be used and it is overlayed onto the final scene.
  //see example of such a function (from another project) commented out at the bottom of this class ...
  
  mDeferredRenderer.setup( fRenderShadowCastersFunc, fRenderNotShadowCastersFunc, NULL, NULL, &mCam, Vec2i(APP_RES_HORIZONTAL, APP_RES_VERTICAL), 1024, true, true, true ); //no overlay or "particles"
//  mDeferredRenderer.setup( fRenderShadowCastersFunc, fRenderNotShadowCastersFunc, fRenderOverlayFunc, NULL, &mCam, Vec2i(APP_RES_HORIZONTAL, APP_RES_VERTICAL), 1024, true, true ); //overlay enabled
  //mDeferredRenderer.setup( fRenderShadowCastersFunc, fRenderNotShadowCastersFunc, fRenderOverlayFunc, fRenderParticlesFunc, &mMayaCam, Vec2i(APP_RES_HORIZONTAL, APP_RES_VERTICAL), 1024, true, true ); //overlay and "particles" enabled -- not working yet
  
  //have these cast point light shadows
  
  Color sun = Color(1.0f, 1.0f, 0.9f);
  mDeferredRenderer.addPointLight( Vec3f(0,0,0), sun * LIGHT_BRIGHTNESS_DEFAULT * 2.0, true);
  
  Color moon = Color(0.24f, 0.15f, 0.94f);
  mDeferredRenderer.addPointLight( Vec3f(0,0,0), moon * LIGHT_BRIGHTNESS_DEFAULT * 0.2, true);
  
  mCurrLightIndex = 0;
  
//  mCurTime = std::time(0);
  count = 0;

  // object loading
  TriMesh       mMesh;
  gl::VboMesh   mVBO;
  
  // time
//  mChronoTimePoint = chrono::high_resolution_clock::now();
//  console() << chrono::duration_cast<chrono::seconds>(mChronoTimePoint.time_since_epoch()).count() << endl;
  
//  std::chrono::time_point<std::chrono::system_clock> p1, now, p3;
//  
//  now = std::chrono::system_clock::now();
//  
//  std::time_t epoch_time = std::chrono::system_clock::to_time_t(p1);
//  std::cout << "epoch: " << std::ctime(&epoch_time);
//  
//  std::time_t today_time = std::chrono::system_clock::to_time_t(now);
//  std::cout << "today: " << std::ctime(&today_time);
//  
//  mTimeHours  = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
//  mTimeHours /= 60; // seconds
//  mTimeHours /= 60; // hours
//  
//  std::cout << "hours since epoch: " << mTimeHours << '\n';
 
  
  // start python
  Py_Initialize();
  // import our script
  py_ephem = PyImport_Import(PyString_FromString((char*)"ephem"));
  py_em = PyImport_Import(PyString_FromString((char*)"ephemScript"));
  
  mTimeHours = getNowHours();
  
  console() << "mTimeHours: " << mTimeHours << endl;
  
//  console() << "getSunPos: " << getSunPosition(mTimeHours) << endl;
}


double LandscapeApp::getNowHours() {
  PyObject* getNowHours = PyObject_GetAttrString(py_em,(char*)"getNowHours");
  PyObject* args = NULL;
  PyObject* myResult = PyObject_CallObject(getNowHours, args);
  return PyFloat_AsDouble(myResult);
}


void LandscapeApp::shutdown()
{
  // end python
  Py_Finalize();
  console() << "done" << std::endl;
}


void LandscapeApp::update()
{
  mCam = mMayaCam.getCamera();
  mDeferredRenderer.mCam = &mCam;
	mCurrFramerate = getAverageFps();

  mTimeHours -= 1.0 / 24.0 / 60.0;
  mSunPos  = getSunPosition(mTimeHours);
  mMoonPos = getMoonPosition(mTimeHours);

  float lightDistance = 14.0f;
  
  // rotate sun
  Vec3f sunPos(lightDistance, 0, 0);
  sunPos.rotate(Vec3f(0, 0, 1), mSunPos.alt );
  sunPos.rotate(Vec3f(0, 1, 0), mSunPos.azm );
  mDeferredRenderer.getCubeLightsRef()->at(0)->setPos(sunPos);
  
  // rotate sun
  Vec3f moonPos(lightDistance, 0, 0);
  moonPos.rotate(Vec3f(0, 0, 1), mMoonPos.alt );
  moonPos.rotate(Vec3f(0, 1, 0), mMoonPos.azm );
  mDeferredRenderer.getCubeLightsRef()->at(1)->setPos(moonPos);
  
}

void LandscapeApp::draw()
{
  mDeferredRenderer.renderFullScreenQuad(RENDER_MODE);
	if (mShowParams) {
		mParams.draw();
  }
}

void LandscapeApp::mouseDown( MouseEvent event )
{
    if( event.isAltDown() ) {
      mMayaCam.mouseDown( event.getPos() );
    }
}

void LandscapeApp::mouseDrag( MouseEvent event )
{
    if( event.isAltDown() ) {
      mMayaCam.mouseDrag( event.getPos(), event.isLeftDown(), event.isMiddleDown(), event.isRightDown() );
    }
}

void LandscapeApp::drawShadowCasters(gl::GlslProg* deferShader) const
{
  gl::pushMatrices();
  gl::scale(1.3f, 1.3f, 1.3f);
  gl::draw( mVBO );
  gl::popMatrices();
}

void LandscapeApp::drawNonShadowCasters(gl::GlslProg* deferShader) const
{
    int size = 10;
    //a plane to capture shadows (though it won't cast any itself)
    glColor3ub(255, 255, 255);
    glNormal3f(0.0f, 1.0f, 0.0f);
    glBegin(GL_QUADS);
    glVertex3i( size, 0,-size);
    glVertex3i(-size, 0,-size);
    glVertex3i(-size, 0, size);
    glVertex3i( size, 0, size);
    glEnd();
}

void LandscapeApp::drawOverlay() const
{
  Vec3f camUp, camRight;
  mCam.getBillboardVectors(&camRight, &camUp);
  
  //create text labels
  TextLayout layout1;
	layout1.clear( ColorA( 1.0f, 1.0f, 1.0f, 0.0f ) );
	layout1.setFont( Font( "Arial", 34 ) );
	layout1.setColor( ColorA( 255.0f/255.0f, 255.0f/255.0f, 8.0f/255.0f, 1.0f ) );
	layout1.addLine( to_string(getAverageFps()) ); //to_string is a c++11 function for conversions
	Surface8u rendered1 = layout1.render( true, false );
  gl::Texture fontTexture_FR = gl::Texture( rendered1 );
  
  //draw framerate
  fontTexture_FR.bind();
  gl::drawBillboard(Vec3f(-3.0f, 7.0f, 0.0f), Vec2f(fontTexture_FR.getWidth()/20.0f , fontTexture_FR.getHeight()/20.0f), 0, camRight, camUp);
  fontTexture_FR.unbind();
}

void LandscapeApp::keyDown( KeyEvent event )
{
  float lightMovInc = 0.25f;
  
	switch ( event.getCode() )
	{
      //switch between render views
		case KeyEvent::KEY_0:
    {RENDER_MODE = DeferredRenderer::SHOW_FINAL_VIEW;}
			break;
		case KeyEvent::KEY_1:
    {RENDER_MODE = DeferredRenderer::SHOW_DIFFUSE_VIEW;}
			break;
		case KeyEvent::KEY_2:
    {RENDER_MODE = DeferredRenderer::SHOW_NORMALMAP_VIEW;}
			break;
		case KeyEvent::KEY_3:
    {RENDER_MODE = DeferredRenderer::SHOW_DEPTH_VIEW;}
			break;
    case KeyEvent::KEY_4:
    {RENDER_MODE = DeferredRenderer::SHOW_POSITION_VIEW;}
			break;
    case KeyEvent::KEY_5:
    {RENDER_MODE = DeferredRenderer::SHOW_ATTRIBUTE_VIEW;}
			break;
    case KeyEvent::KEY_6:
    {RENDER_MODE = DeferredRenderer::SHOW_SSAO_VIEW;}
			break;
    case KeyEvent::KEY_7:
    {RENDER_MODE = DeferredRenderer::SHOW_SSAO_BLURRED_VIEW;}
			break;
    case KeyEvent::KEY_8:
    {RENDER_MODE = DeferredRenderer::SHOW_LIGHT_VIEW;}
      break;
    case KeyEvent::KEY_9:
    {RENDER_MODE = DeferredRenderer::SHOW_SHADOWS_VIEW;}
			break;
      
      //change which cube you want to control
    case 269: {
      //minus key
      if( mDeferredRenderer.getNumCubeLights() > 0) {
        --mCurrLightIndex;
        if ( mCurrLightIndex < 0) mCurrLightIndex = mDeferredRenderer.getNumCubeLights() - 1;
      }
    }
      break;
    case 61: {
      if( mDeferredRenderer.getNumCubeLights() > 0) {
        //plus key
        ++mCurrLightIndex;
        if ( mCurrLightIndex > mDeferredRenderer.getNumCubeLights() - 1) mCurrLightIndex = 0;
      }
    }
      break;
			
      //move selected cube light
		case KeyEvent::KEY_UP: {
      if ( mDeferredRenderer.getNumCubeLights() > 0) {
        if(event.isShiftDown()) {
          mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->setPos( mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->getPos() + Vec3f(0.0f, lightMovInc, 0.0f ));
        }
        else {
          mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->setPos( mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->getPos() + Vec3f(0.0f, 0.0f, lightMovInc));
        }
      }
		}
			break;
		case KeyEvent::KEY_DOWN: {
      if ( mDeferredRenderer.getNumCubeLights() > 0) {
        if(event.isShiftDown()) {
          mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->setPos( mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->getPos() + Vec3f(0.0f, -lightMovInc, 0.0f ));
        }
        else {
          mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->setPos( mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->getPos() + Vec3f(0.0f, 0.0, -lightMovInc));
        }
      }
		}
			break;
		case KeyEvent::KEY_LEFT: {
      if ( mDeferredRenderer.getNumCubeLights() > 0) {
        mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->setPos( mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->getPos() + Vec3f(lightMovInc, 0.0, 0.0f));
      }
		}
			break;
		case KeyEvent::KEY_RIGHT: {
      if ( mDeferredRenderer.getNumCubeLights() > 0) {
        mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->setPos( mDeferredRenderer.getCubeLightsRef()->at(mCurrLightIndex)->getPos() + Vec3f(-lightMovInc, 0.0, 0.0f));
      }
		}
      break;
    case KeyEvent::KEY_ESCAPE: {
      //never know when you need to quit quick
      exit(1);
    }
			break;
		default:
			break;
	}
}


AzmAlt LandscapeApp::getSunPosition(double hours) {
  
  PyObject* getSunAzimuth = PyObject_GetAttrString(py_em, (char*)"getSunAzimuth");
  PyObject* args = PyTuple_Pack(1, PyFloat_FromDouble(hours));
  PyObject* az = PyObject_CallObject(getSunAzimuth, args);

  PyObject* getSunAltitude = PyObject_GetAttrString(py_em, (char*)"getSunAltitude");
  args = PyTuple_Pack(1, PyFloat_FromDouble(hours));
  PyObject* alt = PyObject_CallObject(getSunAltitude, args);

  return { PyFloat_AsDouble(az), PyFloat_AsDouble(alt)};
}
AzmAlt LandscapeApp::getMoonPosition(double hours) {
  PyObject* getMoonAzimuth = PyObject_GetAttrString(py_em, (char*)"getMoonAzimuth");
  PyObject* args = PyTuple_Pack(1, PyFloat_FromDouble(hours));
  PyObject* az = PyObject_CallObject(getMoonAzimuth, args);
  
  PyObject* getMoonAltitude = PyObject_GetAttrString(py_em, (char*)"getMoonAltitude");
  args = PyTuple_Pack(1, PyFloat_FromDouble(hours));
  PyObject* alt = PyObject_CallObject(getMoonAltitude, args);
  
  return {PyFloat_AsDouble(az), PyFloat_AsDouble(alt)};
}




CINDER_APP_BASIC( LandscapeApp, RendererGl )
