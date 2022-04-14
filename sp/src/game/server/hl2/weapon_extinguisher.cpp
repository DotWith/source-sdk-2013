//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#include "cbase.h"
#include "basehlcombatweapon.h"
#include "gamerules.h"
#include "basecombatcharacter.h"
#include "game.h"
#include "in_buttons.h"
#include "extinguisherjet.h"
#include "entitylist.h"
#include "fire.h"
#include "ar2_explosion.h"
#include "soundent.h"
#include "engine/IEngineSound.h"
#include "gamestats.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

ConVar	fire_extinguisher_radius( "fire_extinguisher_radius", "45" );
ConVar	fire_extinguisher_distance( "fire_extinguisher_distance", "200" );
ConVar	fire_extinguisher_strength( "fire_extinguisher_strength", "0.97" );	//TODO: Stub for real numbers
ConVar	fire_extinguisher_explode_radius( "fire_extinguisher_explode_radius", "256" );
ConVar	fire_extinguisher_explode_strength( "fire_extinguisher_explode_strength", "0.3" );	//TODO: Stub for real numbers

#define	EXTINGUISHER_AMMO_RATE	0.2
#define	EXTINGUISHER_SPAWN_HEALTH 25

extern short	g_sModelIndexFireball;	// (in combatweapon.cpp) holds the index for the smoke cloud

class CWeaponExtinguisher : public CHLSelectFireMachineGun
{
	DECLARE_DATADESC();
public:
	DECLARE_CLASS( CWeaponExtinguisher, CHLSelectFireMachineGun );

	DECLARE_SERVERCLASS();

	CWeaponExtinguisher();

	void	Spawn( void );
	void	Precache( void );

	void	PrimaryAttack( void );

	void	ItemPostFrame( void );
	void	Event_Killed( const CTakeDamageInfo &info );
	void	Equip( CBaseCombatCharacter *pOwner );

	static CExtinguisherJet* Create( const Vector& vecOrigin, const QAngle& vecAngles, edict_t* pentOwner );

protected:
	
	void	StartJet( void );
	void	StopJet( void );

	CExtinguisherJet	*m_pJet;
};

IMPLEMENT_SERVERCLASS_ST( CWeaponExtinguisher, DT_WeaponExtinguisher )
END_SEND_TABLE()

LINK_ENTITY_TO_CLASS( weapon_extinguisher, CWeaponExtinguisher );
PRECACHE_WEAPON_REGISTER( weapon_extinguisher );

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CWeaponExtinguisher )

	DEFINE_FIELD( m_pJet,	FIELD_CLASSPTR ),

END_DATADESC()

CWeaponExtinguisher::CWeaponExtinguisher( void )
{
	m_pJet = NULL;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::Precache( void )
{
	PrecacheScriptSound( "ExtinguisherJet.TurnOn" );
	PrecacheScriptSound( "ExtinguisherJet.TurnOff" );

	UTIL_PrecacheOther( "env_extinguisherjet" );

	BaseClass::Precache();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::PrimaryAttack( void )
{
	CBasePlayer* pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// Turn the jet on
	StartJet();

	Vector vForward, vRight, vUp;

	pOwner->EyeVectors( &vForward, &vRight, &vUp );

	Vector muzzlePoint = pOwner->Weapon_ShootPosition() + vForward * 15.0f + vRight * 6.0f + vUp * -4.0f;

	trace_t	tr;
	Vector vecEye = pOwner->EyePosition();
	UTIL_TraceLine( vecEye, vecEye + vForward * 128, MASK_SHOT, this, COLLISION_GROUP_NONE, &tr );

	if ( m_flNextPrimaryAttack < gpGlobals->curtime )
	{
		// Take away our primary ammo type
		pOwner->RemoveAmmo( 1, m_iPrimaryAmmoType );
		m_flNextPrimaryAttack = gpGlobals->curtime + EXTINGUISHER_AMMO_RATE;
	}

	// Extinguish the fire where we hit
	FireSystem_ExtinguishInRadius( tr.endpos, fire_extinguisher_radius.GetFloat(), fire_extinguisher_strength.GetFloat() );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::Spawn( void )
{
	BaseClass::Spawn();

	m_takedamage	= DAMAGE_YES;
	m_iHealth		= EXTINGUISHER_SPAWN_HEALTH;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pOwner - 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::Equip( CBaseCombatCharacter *pOwner )
{
	BaseClass::Equip( pOwner );

	m_takedamage	= DAMAGE_NO;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pInflictor - 
//			*pAttacker - 
//			flDamage - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::Event_Killed( const CTakeDamageInfo &info )
{
	// TODO: Use a real effect
	if ( AR2Explosion *pExplosion = AR2Explosion::CreateAR2Explosion( GetAbsOrigin() ) )
	{
		pExplosion->SetLifetime( 10 );
	}		

	// TODO: Use a real effect
	CPASFilter filter( GetAbsOrigin() );

	te->Explosion( filter, 0.0,
		&GetAbsOrigin(), 
		g_sModelIndexFireball,
		2.0, 
		15,
		TE_EXPLFLAG_NONE,
		250,
		100 );

	// Put out fire in a radius
	FireSystem_ExtinguishInRadius( GetAbsOrigin(), fire_extinguisher_explode_radius.GetFloat(), fire_extinguisher_explode_strength.GetFloat() );

	SetThink( &CWeaponExtinguisher::SUB_Remove );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

//-----------------------------------------------------------------------------
// Purpose: Turn the jet effect and noise on
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::StartJet( void )
{
	WeaponSound( SINGLE );

	// See if the jet needs to be created
	if ( m_pJet == NULL )
	{
		m_pJet = CWeaponExtinguisher::Create( GetAbsOrigin(), GetAbsAngles(), GetOwner()->edict() );
	}

	// Turn the jet on
	if ( m_pJet != NULL )
	{
		m_pJet->TurnOn( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : &vecOrigin - 
//			&vecAngles - 
//			NULL - 
//
// Output : CExtinguisherJet
//-----------------------------------------------------------------------------
CExtinguisherJet * CWeaponExtinguisher::Create( const Vector& vecOrigin, const QAngle& vecAngles, edict_t* pentOwner = NULL )
{
	CExtinguisherJet* pJet = (CExtinguisherJet *) CBaseEntity::Create( "env_extinguisherjet", vecOrigin, vecAngles, CBaseEntity::Instance( pentOwner ) );

	pJet->SetOwnerEntity( Instance( pentOwner ) );
	pJet->Spawn();

	pJet->m_bEmit = false;
	pJet->m_bUseMuzzlePoint = true;
	pJet->m_bAutoExtinguish = false;
	pJet->m_nLength = fire_extinguisher_distance.GetInt();

	return pJet;
}

//-----------------------------------------------------------------------------
// Purpose: Turn the jet effect and noise off
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::StopJet( void )
{
	// Turn the jet off
	if ( m_pJet != NULL )
	{
		m_pJet->TurnOff( this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CWeaponExtinguisher::ItemPostFrame( void )
{
	CBasePlayer* pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	//See if we should try and extinguish fires
	if ( pOwner->m_nButtons & IN_ATTACK )
	{
		if ( !IsMeleeWeapon() &&
			( ( UsesClipsForAmmo1() && m_iClip1 <= 0 ) || ( !UsesClipsForAmmo1() && pOwner->GetAmmoCount( m_iPrimaryAmmoType ) <= 0 ) ) )
		{
			HandleFireOnEmpty();
		}
		else if ( pOwner->GetWaterLevel() == 3 && m_bFiresUnderwater == false )
		{
			// This weapon doesn't fire underwater
			WeaponSound( EMPTY );
			m_flNextPrimaryAttack = gpGlobals->curtime + 0.2;
			return;
		}
		else
		{
			PrimaryAttack();
		}
	}
	else
	{
		StopJet();
	}
}

class CExtinguisherCharger : public CBaseToggle
{
public:
	DECLARE_CLASS( CExtinguisherCharger, CBaseToggle );

	virtual void	Precache( void );
	virtual void	Spawn( void );
	bool CreateVPhysics();
	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );

	virtual int	ObjectCaps( void ) { return (BaseClass::ObjectCaps() | FCAP_CONTINUOUS_USE) & ~FCAP_ACROSS_TRANSITION; }

protected:
	float	m_flNextCharge; 
	bool	m_bSoundOn;

	virtual void	TurnOff( void );

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CLASS( func_extinguishercharger, CExtinguisherCharger );

BEGIN_DATADESC( CExtinguisherCharger )

	DEFINE_FIELD( m_flNextCharge, FIELD_TIME),
	DEFINE_FIELD( m_bSoundOn, FIELD_BOOLEAN ),

	DEFINE_FUNCTION( TurnOff ),

END_DATADESC()

void CExtinguisherCharger::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound("ExtinguisherCharger.Use");
}

//-----------------------------------------------------------------------------
// Purpose: Spawn the object
//-----------------------------------------------------------------------------
void CExtinguisherCharger::Spawn( void )
{
	Precache();

	SetSolid( SOLID_BSP );
	SetMoveType( MOVETYPE_PUSH );

	SetModel( STRING( GetModelName() ) );

	m_bSoundOn = false;

	CreateVPhysics();
}

//-----------------------------------------------------------------------------

bool CExtinguisherCharger::CreateVPhysics()
{
	VPhysicsInitStatic();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CExtinguisherCharger::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	// Make sure that we have a caller
	if ( pActivator == NULL )
		return;
	
	// If it's not a player, ignore
	if ( pActivator->IsPlayer() == false )
		return;

	// Turn our sound on, if it's not already
	if ( m_bSoundOn == false )
	{
		EmitSound( "ExtinguisherCharger.Use" );
		m_bSoundOn = true;
	}

	SetNextThink( gpGlobals->curtime + 0.25 );
	
	SetThink( &CExtinguisherCharger::TurnOff );

	CBasePlayer* pPlayer = ToBasePlayer( pActivator );

	if ( pPlayer )
	{
		pPlayer->GiveAmmo( 1, "extinguisher", true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CExtinguisherCharger::TurnOff( void )
{
	//Turn the sound off
	if ( m_bSoundOn )
	{
		StopSound( "ExtinguisherCharger.Use" );
		m_bSoundOn = false;
	}
}
