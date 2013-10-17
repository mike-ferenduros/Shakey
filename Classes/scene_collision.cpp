#include "scene.h"
#include <ode/ode.h>

#define MAX_CONTACTS		4

void Scene::collision_init( void )
{
	m_dspace = dSimpleSpaceCreate( 0 );
	m_colljoints = dJointGroupCreate( 0 );

	const dReal sideplanes[6][4] =
	{
		{ 1,0,0, -160 }, { -1,0,0, -160 },		//sides
		{ 0,1,0, -240 }, { 0,-1,0, -240 },		//top and bottom
		{ 0,0,1, -50  }, { 0,0,-1, -50 },		//back and front planes
	};
	for( int i = 0; i < 6; i++ )
	{
		const dReal *p = sideplanes[i];
		m_sidewalls[i] = dCreatePlane( m_dspace, p[0], p[1], p[2], p[3] );
		dGeomSetData( m_sidewalls[i], (void*)i );
	}
}



void Scene::collision_shutdown( void )
{
	for( int i = 0; i < 6; i++ )
	{
		dGeomDestroy( m_sidewalls[i] );
	}

	dJointGroupDestroy( m_colljoints );

	dSpaceDestroy( m_dspace );
}


static void near_collide( void *vscene_p, dGeomID dgeom0, dGeomID dgeom1 )
{
	dBodyID dbody0 = dGeomGetBody( dgeom0 );
	dBodyID dbody1 = dGeomGetBody( dgeom1 );

	//Ignore static-static collisions
	if( !(dbody0 || dbody1) )
		return;



	dContactGeom contact_geoms[MAX_CONTACTS];
	int num_contacts = dCollide( dgeom0, dgeom1, MAX_CONTACTS, contact_geoms, sizeof(dContactGeom) );
	if( num_contacts )
	{
		Scene *scene_p = (Scene*)vscene_p;
		dWorldID dworld = scene_p->m_dworld;
		dJointGroupID colljoints = scene_p->m_colljoints;

		dContact contact;
		contact.surface.mode = dContactBounce | dContactSoftCFM;
		if( dbody0 && dbody1 )
		{
			//Object-to-object
			contact.surface.mu = 0;
			contact.surface.bounce = 0.4;
			contact.surface.bounce_vel = 0.1;
			contact.surface.soft_cfm = 0.05f;
		}
		else
		{
			//Object-to-world
			contact.surface.mu = 100;
			contact.surface.bounce = 0.2;
			contact.surface.bounce_vel = 0.1;
			contact.surface.soft_cfm = 0.01f;
		}

		for( int c = 0; c < num_contacts; c++ )
		{
			contact.geom = contact_geoms[c];
			dJointID cjoint = dJointCreateContact( dworld, colljoints, &contact );
			dJointAttach( cjoint, dbody0, dbody1 );
		}
	}
}


void Scene::do_collision( void )
{
	dJointGroupEmpty( m_colljoints );
	dSpaceCollide( m_dspace, (void*)this, near_collide );
}



static float frandy( float maxf )
{
	int i = random() % 10001;
	return( float(i) * maxf * 0.0001f );
}

static void wakey_wakey( void *nothing, dGeomID dgeom0, dGeomID dgeom1 )
{
	dBodyID dbody0 = dGeomGetBody( dgeom0 );
	dBodyID dbody1 = dGeomGetBody( dgeom1 );

	if( dbody0 )
	{
		dBodyEnable( dbody0 );
		dBodyAddForce( dbody0, frandy(1000)-500, 2000, frandy(1000)-500 );
		dBodyAddTorque( dbody0, frandy(50000)-25000, frandy(10000)-5000, frandy(10000)-5000 );
	}

	if( dbody1 )
	{
		dBodyEnable( dbody1 );
		dBodyAddForce( dbody1, frandy(1000)-500, 2000, frandy(1000)-500 );
		dBodyAddTorque( dbody1, frandy(50000)-25000, frandy(10000)-5000, frandy(10000)-5000 );
	}
}

void Scene::touch( float x, float y )
{
	dGeomID dray = dCreateRay( 0, 2000.0f );
	dGeomSetPosition( dray, x-160.0f, 240-y, -1000.0f );

	dSpaceCollide2( dray, (dGeomID)m_dspace, 0, wakey_wakey );
	
	dGeomDestroy( dray );
}
