#ifndef ISOSURFACE_H
#define ISOSURFACE_H

#include "xbsBase.h"

class IsoSurface
{
public:
    IsoSurface();
    ~IsoSurface();
    
	void Init( LPDIRECT3DDEVICE8 pd3dDevice );
	virtual float Sample( float mx, float my, float mz );    
    void March();
	void Render( LPDIRECT3DDEVICE8 pd3dDevice );
	void SetDensity( int density );
        
    float m_TargetValue;
    D3DXVECTOR3 *m_pVxs;
    D3DXVECTOR3 *m_pNorms;
    int m_iVxCount;
    int m_iFaceCount;
    
private:    
	void GetNormal( D3DXVECTOR3 &normal, D3DXVECTOR3 &position );
    void MarchCube( float mx, float my, float mz, float scale );
    float GetOffset( float val1, float val2, float wanted );
    
    LPDIRECT3DVERTEXBUFFER8 m_pVertexBuffer;
	int m_DatasetSize;
	float m_StepSize;	
};    
        
#endif
