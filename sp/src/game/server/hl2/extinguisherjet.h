//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef ENTITYEXTINGUISHERJET_H
#define ENTITYEXTINGUISHERJET_H

#ifdef _WIN32
#pragma once
#endif

#include "baseparticleentity.h"

class CExtinguisherJet : public CBaseEntity
{
public:	
	DECLARE_SERVERCLASS();
	DECLARE_CLASS( CExtinguisherJet, CBaseEntity );

	CExtinguisherJet();

	virtual void	Spawn( void );
	virtual void	Precache();

	virtual void	TurnOn( CBaseCombatWeapon *weapon );
	virtual void	TurnOff( CBaseCombatWeapon* weapon );

	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );
	void	InputToggle( inputdata_t &inputdata );

	virtual void Think( void );
	
	virtual void ExtinguishThink( void );

	DECLARE_DATADESC();

// Stuff from the datatable.
public:
	CNetworkVar( bool, m_bEmit );	// Emit particles?
	CNetworkVar( int, m_nLength );	// Length of jet
	CNetworkVar( int, m_nSize );	// Size of jet (as in width and noise of particle movement)
	int		m_nRadius;	// Radius area to extinguish where jet hits
	float	m_flStrength;	// Strength of the extinguisher

	bool	m_bEnabled;
	
	//Used for viewmodel
	CNetworkVar( bool, m_bUseMuzzlePoint );
	bool	m_bAutoExtinguish;	//Whether extinguisher should put out fires in its think, or let owner do it
};

#endif // ENTITYEXTINGUISHERJET_H
