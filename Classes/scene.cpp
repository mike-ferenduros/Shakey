#include "scene.h"
#include "pvrtex.h"
#include <ode/ode.h>

#include <OpenGLES/ES1/gl.h>

int Scene::m_instance_count = 0;

static const GLfloat box_xyz[8][3] =
{
	{ -1, -1, -1 }, { -1, -1,  1 }, { -1,  1, -1 }, { -1,  1,  1 },
	{  1, -1, -1 }, {  1, -1,  1 }, {  1,  1, -1 }, {  1,  1,  1 },
};

static const GLfloat box_st[8][2] =
{
	{  1.0f/64.0f, 58.0f/64.0f }, {  1.0f/64.0f, 58.0f/64.0f }, {  1.0f/64.0f, 1.0f/64.0f }, {  1.0f/64.0f, 1.0f/64.0f },
	{ 58.0f/64.0f, 58.0f/64.0f }, { 58.0f/64.0f, 58.0f/64.0f }, { 58.0f/64.0f, 1.0f/64.0f }, { 58.0f/64.0f, 1.0f/64.0f }, 
};

static const GLubyte box_idx[36] =
{
	0,1,3, 0,3,2,	4,6,7, 4,7,5,	0,4,5, 0,5,1,
	3,7,6, 3,6,2,	0,2,6, 0,6,4,	1,5,7, 7,3,1,
};


SceneObj::SceneObj( Scene *scene_p )
{
	m_scene_p = scene_p;
	m_prev_p = 0;
	m_next_p = m_scene_p->m_obj_p;
	if( m_next_p )
		m_next_p->m_prev_p = this;
	m_scene_p->m_obj_p = this;
	
	m_dbody = dBodyCreate( m_scene_p->m_dworld );
	dBodySetData( m_dbody, this );
	dBodyDisable( m_dbody );

	dMass mass;
	dMassSetBoxTotal( &mass, 0.1f, 57, 57, 20 );
	dBodySetMass( m_dbody, &mass );

	m_dgeom = dCreateBox( m_scene_p->m_dspace, 57, 57, 20 );
	dGeomSetBody( m_dgeom, m_dbody );
	
	m_texid = 0;
}

SceneObj::~SceneObj( void )
{
	if( m_dgeom )
		dGeomDestroy( m_dgeom );

	if( m_dbody )
		dBodyDestroy( m_dbody );

	if( m_prev_p )		m_prev_p->m_next_p = m_next_p;
	if( m_next_p )		m_next_p->m_prev_p = m_prev_p;

	if( m_scene_p->m_obj_p==this )
		m_scene_p->m_obj_p = m_next_p;
}

void SceneObj::render( void )
{
	GLenum err;
	if( (err=glGetError()) != 0 )
	{
		fprintf( stderr, "GLERR %x\n", err );
	}

	glMatrixMode( GL_MODELVIEW );
	glPushMatrix();

	const dReal *pos = dBodyGetPosition( m_dbody );
	const dReal *rot = dBodyGetRotation( m_dbody );
	GLfloat mat[16];
	mat[ 0] = rot[ 0];	mat[ 1] = rot[ 4];	mat[ 2] = rot[ 8];	mat[ 3] = 0;
	mat[ 4] = rot[ 1];	mat[ 5] = rot[ 5];	mat[ 6] = rot[ 9];	mat[ 7] = 0;
	mat[ 8] = rot[ 2];	mat[ 9] = rot[ 6];	mat[10] = rot[10];	mat[11] = 0;
	mat[12] = pos[ 0];	mat[13] = pos[ 1];	mat[14] = pos[ 2];	mat[15] = 1;
	glMultMatrixf( mat );

	dVector3 dims;
	dGeomBoxGetLengths( m_dgeom, dims );
	glScalef( dims[0]*0.5f, dims[1]*0.5f, dims[2]*0.5f );

    glVertexPointer( 3, GL_FLOAT, 0, box_xyz );
    glEnableClientState( GL_VERTEX_ARRAY );

 	glTexCoordPointer( 2, GL_FLOAT, 0, box_st );
    glEnableClientState( GL_TEXTURE_COORD_ARRAY );

	glEnable( GL_TEXTURE_2D );
	glBindTexture( GL_TEXTURE_2D, m_scene_p->m_textures[m_texid] );

	glDrawElements( GL_TRIANGLES, 36, GL_UNSIGNED_BYTE, box_idx );

	glDisable( GL_TEXTURE_2D );

	glPopMatrix();
}


Scene::Scene( void )
{
	if( m_instance_count == 0 )
		dInitODE2( 0 );
	m_instance_count++;

	m_obj_p = 0;
	
	m_dworld = dWorldCreate();
	
	collision_init();

	for( int x = 0; x < 4; x++ )
	{
		for( int y = 0; y < 4; y++ )
		{
			float xpos = -160 + float(x)*76.0f + 46.5f;
			float ypos =  240 - float(y)*88.0f - 61.5f;
			SceneObj *arse_p = new SceneObj( this );
			dBodySetPosition( arse_p->m_dbody, xpos, ypos, 0.0f );
			arse_p->m_texid = x + y*4;
		}
	}

	m_motion[0] = 0;		m_motion_lopass[0] = 0;		m_motion_hipass[0] = 0;
	m_motion[1] = 0;		m_motion_lopass[1] = 0;		m_motion_hipass[1] = 0;
	m_motion[2] = 0;		m_motion_lopass[2] = 0;		m_motion_hipass[2] = 0;

	for( int t = 0; t < 16; t++ )
		m_textures[t] = 0;
}

Scene::~Scene( void )
{
	while( m_obj_p )
		delete m_obj_p;
	
	collision_shutdown();

	dWorldDestroy( m_dworld );

	for( int t = 0; t < 16; t++ )
	{
		GLuint texid = m_textures[t];
		glDeleteTextures( 1, &texid );
	}

	m_instance_count--;
	if( m_instance_count == 0 )
		dCloseODE();
}

void Scene::motion( float x, float y, float z )
{
	m_motion_lopass[0] = m_motion[0] + (x-m_motion[0])*0.1f;
	m_motion_lopass[1] = m_motion[1] + (y-m_motion[1])*0.1f;
	m_motion_lopass[2] = m_motion[2] + (z-m_motion[2])*0.1f;

	m_motion_hipass[0] = x - m_motion_lopass[0];
	m_motion_hipass[1] = y - m_motion_lopass[1];
	m_motion_hipass[2] = z - m_motion_lopass[2];

	m_motion[0] = x;
	m_motion[1] = y;
	m_motion[2] = z;
}



void Scene::render( void )
{
	if( m_textures[0] == 0 )
	{
		m_textures[ 0] = load_pvrtex( "add_ico" );
		m_textures[ 1] = load_pvrtex( "app_ico" );
		m_textures[ 2] = load_pvrtex( "cal_ico" );
		m_textures[ 3] = load_pvrtex( "clc_ico" );
		m_textures[ 4] = load_pvrtex( "clo_ico" );
		m_textures[ 5] = load_pvrtex( "ear_ico" );
		m_textures[ 6] = load_pvrtex( "itu_ico" );
		m_textures[ 7] = load_pvrtex( "map_ico" );
		m_textures[ 8] = load_pvrtex( "mov_ico" );
		m_textures[ 9] = load_pvrtex( "not_ico" );
		m_textures[10] = load_pvrtex( "pho_ico" );
		m_textures[11] = load_pvrtex( "saf_ico" );
		m_textures[12] = load_pvrtex( "set_ico" );
		m_textures[13] = load_pvrtex( "sto_ico" );
		m_textures[14] = load_pvrtex( "wea_ico" );
		m_textures[15] = load_pvrtex( "you_ico" );
	}
	
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrthof( -160, 160, -240, 240, -100, 100 );

    glClearColor( 0, 0, 0, 1 );
    glClear( GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT );

	glEnable( GL_DEPTH_TEST );
	glDepthFunc( GL_LEQUAL );

	for( SceneObj *obj_p=m_obj_p; obj_p; obj_p=obj_p->m_next_p )
	{
		obj_p->render();
	}
}

void Scene::step( void )
{
	//Our internal units are pixels, and at 160dpi there are 6300 pixels per meter
	float gravity[3];
	gravity[0] = (m_motion_lopass[0]*1 + m_motion_hipass[0]*10) * 6300;
	gravity[1] = (m_motion_lopass[1]*1 + m_motion_hipass[1]*10) * 6300;
	gravity[2] = (m_motion_lopass[2]*1 + m_motion_hipass[2]*10) * 6300;
	
	float hisq = m_motion_hipass[0]*m_motion_hipass[0] + m_motion_hipass[1]*m_motion_hipass[1] + m_motion_hipass[2]*m_motion_hipass[2];
	if( hisq > 1.0f )
	{
		int num_woken = 0;
		for( SceneObj *obj_p=m_obj_p; obj_p; obj_p=obj_p->m_next_p )
		{
			if( !dBodyIsEnabled(obj_p->m_dbody) )
			{
				dBodyEnable( obj_p->m_dbody );
				num_woken++;
				if( num_woken == 3 )
					break;
			}
		}
	}

	dWorldSetGravity( m_dworld, gravity[0], gravity[1], gravity[2] );

	do_collision();
	dWorldQuickStep( m_dworld, 1.0f/60.0f );
	dJointGroupEmpty( m_colljoints );
}
