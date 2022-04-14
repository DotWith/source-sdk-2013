//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//

#ifndef C_ENTITY_EXTINGUISHERJET_H
#define C_ENTITY_EXTINGUISHERJET_H

#include "cbase.h"

//-----------------------------------------------------------------------------
// Entity ExtinguisherJet, client-side implementation
//-----------------------------------------------------------------------------
class C_ExtinguisherJet : public C_BaseEntity
{
public:
	DECLARE_CLIENTCLASS();
	DECLARE_CLASS( C_ExtinguisherJet, C_BaseEntity );

	C_ExtinguisherJet( void );

	void	OnDataChanged( DataUpdateType_t updateType );
	void	Update( float fTimeDelta );
	void	Start( void );
	int		DrawModel( int flags );
	bool	ShouldDraw( void ) { return m_bEmit; }

protected:

	void						AddExtinguisherDecal( trace_t &tr );

	bool						m_bEmit;
	bool						m_bUseMuzzlePoint;
	int							m_nLength;
	int							m_nSize;

	PMaterialHandle				m_MaterialHandle;
	PMaterialHandle				m_EmberMaterialHandle;
	TimedEvent					m_ParticleSpawn;
	CSmartPtr<CSimpleEmitter>	m_pEmitter;
	CSmartPtr<CEmberEffect>		m_pEmberEmitter;
};

#endif // C_ENTITY_EXTINGUISHERJET_H
