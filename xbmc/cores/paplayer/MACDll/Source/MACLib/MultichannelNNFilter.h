#ifndef APE_MULTICHANNEL_NNFILTER_H
#define APE_MULTICHANNEL_NNFILTER_H

#include "NNFilter.h"

class CMultichannelNNFilter
{
public:

	CMultichannelNNFilter(int nOrder1, int nOrder2, int nShift)
	{
		m_pNNFilterA = new CNNFilter(nOrder1 + nOrder2, 11, 3980);
		m_pNNFilterB = new CNNFilter(nOrder1 + nOrder2, 11, 3980);
		
		m_rbA.Create(NN_WINDOW_ELEMENTS, nOrder1 + nOrder2 + 1);
		m_rbB.Create(NN_WINDOW_ELEMENTS, nOrder1 + nOrder2 + 1);

		m_nShift = nShift;

		m_nOrder1 = nOrder1;
	}


	~CMultichannelNNFilter()
	{
		SAFE_DELETE(m_pNNFilterA)
		SAFE_DELETE(m_pNNFilterB)
	}

	void Flush()
	{
		m_pNNFilterA->Flush();
		m_pNNFilterB->Flush();

		m_rbA.Flush();
		m_rbB.Flush();

	}

	inline void Compress(int & nA, int & nB)
	{
		if (m_nShift <= 0)
			return;

		m_rbA[0] = GetSaturatedShortFromInt(nA); m_rbB[0] = GetSaturatedShortFromInt(nB);
		m_rbA[-m_nOrder1 - 1] = m_rbB[-1]; m_rbB[-m_nOrder1 - 1] = m_rbA[0];

		nA -= (m_pNNFilterA->GetPrediction(&m_rbA[-1]) >> m_nShift);
		nB -= (m_pNNFilterB->GetPrediction(&m_rbB[-1]) >> m_nShift);

		m_pNNFilterA->AdaptAfterPrediction(&m_rbA[-1], -m_nOrder1, nA);
		m_pNNFilterB->AdaptAfterPrediction(&m_rbB[-1], -m_nOrder1, nB);

		m_rbA.IncrementSafe(); m_rbB.IncrementSafe();
	}

	inline void Decompress(int & nA, int & nB)
	{
		if (m_nShift <= 0)
			return;

		m_rbA[-m_nOrder1 - 1] = m_rbB[-1];
		int nOutputA = nA + (m_pNNFilterA->GetPrediction(&m_rbA[-1]) >> m_nShift);
		m_rbA[0] = GetSaturatedShortFromInt(nOutputA);

		m_rbB[-m_nOrder1 - 1] = m_rbA[0];
		int nOutputB = nB + (m_pNNFilterB->GetPrediction(&m_rbB[-1]) >> m_nShift);
		m_rbB[0] = GetSaturatedShortFromInt(nOutputB); 

		m_pNNFilterA->AdaptAfterPrediction(&m_rbA[-1], -m_nOrder1, nA);
		m_pNNFilterB->AdaptAfterPrediction(&m_rbB[-1], -m_nOrder1, nB);

		m_rbA.IncrementSafe(); m_rbB.IncrementSafe();

		nA = nOutputA; nB = nOutputB;
	}

protected:

	CNNFilter * m_pNNFilterA;
	CNNFilter * m_pNNFilterB;

	int m_nShift;
	int m_nOrder1;

	CRollBuffer<short> m_rbA;
	CRollBuffer<short> m_rbB;

	inline short GetSaturatedShortFromInt(int nValue) const
	{
		return short((nValue == short(nValue)) ? nValue : (nValue >> 31) ^ 0x7FFF);
	}
};

#endif // #ifndef APE_MULTICHANNEL_NNFILTER_H
