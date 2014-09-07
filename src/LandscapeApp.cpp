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

using namespace ci;
using namespace ci::app;
using namespace std;


static const float	APP_RES_HORIZONTAL = 1024.0f;
static const float	APP_RES_VERTICAL = 768.0f;
static const Vec3f	CAM_POSITION_INIT( -14.0f, 7.0f, -14.0f );
static const Vec3f	LIGHT_POSITION_INIT( 3.0f, 1.5f, 0.0f );

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
  
    void initPython();
  
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
    Vec2<double> getSunPosition(long UTCtimestamp);
    Vec2<double> getMoonPosition(long UTCtimestamp);
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
  Color white = Color(1.0f, 1.0f, 1.0f);
  Color blue = Color(0.2f, 1.0f, 1.0f);

  mDeferredRenderer.addPointLight(    Vec3f(-2.0f, 4.0f, 6.0f),
                                  blue * LIGHT_BRIGHTNESS_DEFAULT * 0.65, true);      //blue

  mDeferredRenderer.addPointLight(    Vec3f(4.0f, 6.0f, -4.0f),      Color(0.94f, 0.15f, 0.23f) * LIGHT_BRIGHTNESS_DEFAULT, true);      //red

    
  mCurrLightIndex = 0;
  
  

  // object loading
  TriMesh       mMesh;
  gl::VboMesh   mVBO;
  
  initPython();
}

void LandscapeApp::shutdown()
{
  // end python
  Py_Finalize();
  console() << "done" << std::endl;
}

void LandscapeApp::initPython() {
  
  /////////////////////////////////////
  // python stuff
  fs::path ephemScriptPath = getResourcePath(RES_EPHEM_SCRIPT);
  
  Py_Initialize();
  
  PyRun_SimpleString("import imp");
  PyRun_SimpleString("import ephem");
  char cmd [200];
  sprintf (cmd, "ephemScript = imp.load_source('ephemScript', '%s')", ephemScriptPath.c_str());
  PyRun_SimpleString(cmd);
  

  console() << "sun: " << getSunPosition(1410126987) << endl;
  console() << "moon: " << getMoonPosition(1410126987) << endl;
  

}

Vec2<double> LandscapeApp::getSunPosition(long UTCtimestamp) {
  
  // this is ridiculously messy, just hacking it to make it work atm x_x
  PyRun_SimpleString("result = ephemScript.getAzimuth(ephem.Sun(), ephem.now())");
  PyObject * module = PyImport_AddModule("__main__"); // borrowed reference
  assert(module);                                     // __main__ should always exist
  PyObject * dictionary = PyModule_GetDict(module);   // borrowed reference
  assert(dictionary);                                 // __main__ should have a dictionary
  PyObject * result = PyDict_GetItemString(dictionary, "result");     // borrowed reference
  assert(result);                                     // just added result
  assert(PyFloat_Check(result));                      // result should be a float
  float azimuth = PyFloat_AsDouble(result);          // already checked that it is an int
  
  PyRun_SimpleString("result2 = ephemScript.getAltitude(ephem.Sun(), ephem.now())");
  PyObject * result2 = PyDict_GetItemString(dictionary, "result2");     // borrowed reference
  assert(result2);                                     // just added result
  assert(PyFloat_Check(result2));                      // result should be a float
  float altitude = PyFloat_AsDouble(result2);          // already checked that it is an int
  
  return Vec2<double>(azimuth, altitude);
}
Vec2<double> LandscapeApp::getMoonPosition(long UTCtimestamp) {
  
  // this is ridiculously messy, just hacking it to make it work atm x_x
  PyRun_SimpleString("result = ephemScript.getAzimuth(ephem.Moon(), ephem.now())");
  PyObject * module = PyImport_AddModule("__main__"); // borrowed reference
  assert(module);                                     // __main__ should always exist
  PyObject * dictionary = PyModule_GetDict(module);   // borrowed reference
  assert(dictionary);                                 // __main__ should have a dictionary
  PyObject * result = PyDict_GetItemString(dictionary, "result");     // borrowed reference
  assert(result);                                     // just added result
  assert(PyFloat_Check(result));                      // result should be a float
  float azimuth = PyFloat_AsDouble(result);          // already checked that it is an int
  
  PyRun_SimpleString("result2 = ephemScript.getAltitude(ephem.Moon(), ephem.now())");
  PyObject * result2 = PyDict_GetItemString(dictionary, "result2");     // borrowed reference
  assert(result2);                                     // just added result
  assert(PyFloat_Check(result2));                      // result should be a float
  float altitude = PyFloat_AsDouble(result2);          // already checked that it is an int
  
  return Vec2<double>(azimuth, altitude);
}


void LandscapeApp::update()
{
  mCam = mMayaCam.getCamera();
  mDeferredRenderer.mCam = &mCam;
	mCurrFramerate = getAverageFps();
}

void LandscapeApp::draw()
{
  mDeferredRenderer.renderFullScreenQuad(RENDER_MODE);
	if (mShowParams) {
		mParams.draw();
  }
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
    int size = 3000;
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






CINDER_APP_BASIC( LandscapeApp, RendererGl )
