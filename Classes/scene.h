#ifndef SCENE_H_INCLUDED
#define SCENE_H_INCLUDED

#include <ode/ode.h>

class Scene;
class SceneObj;


class SceneObj
{
friend class Scene;
private:
	SceneObj( Scene *scene_p );
	~SceneObj( void );

	Scene *			m_scene_p;
	SceneObj *		m_prev_p;
	SceneObj *		m_next_p;

	dBodyID			m_dbody;
	dGeomID			m_dgeom;
	
	int				m_texid;
	
	void render( void );
};



static void near_collide( void *vscene_p, dGeomID dgeom0, dGeomID dgeom1 );

class Scene
{
friend class SceneObj;
friend void near_collide( void *, dGeomID, dGeomID );
public:
	Scene( void );
	~Scene( void );

	SceneObj *		m_obj_p;

	void render( void );
	void step( void );
	void motion( float x, float y, float z );
	void touch( float x, float y );

private:
	static int		m_instance_count;

	int				m_textures[16];

	float			m_motion[3];
	float			m_motion_lopass[3];
	float			m_motion_hipass[3];

	dWorldID		m_dworld;

	void collision_init( void );
	void collision_shutdown( void );
	void do_collision( void );
	dSpaceID		m_dspace;
	dJointGroupID	m_colljoints;
	dGeomID			m_sidewalls[6];
};


#endif
