#ifndef APE_START_FILTER_H
#define APE_START_FILTER_H

class CStartFilter
{
public:

	CStartFilter()
	{

	}

	~CStartFilter()
	{


	}

	void Flush()
	{
		m_rbInputA.Flush();
		m_rbInputB.Flush();

		memset(m_aryMA, 0, sizeof(m_aryMA));
		memset(m_aryMB, 0, sizeof(m_aryMB));

		m_Stage1FilterA1.Flush();
		m_Stage1FilterA2.Flush();
		m_Stage1FilterA3.Flush();

		m_Stage1FilterB1.Flush();
		m_Stage1FilterB2.Flush();
		m_Stage1FilterB3.Flush();
	}

	void Compress(int & nA, int & nB)
	{
/*
		nA = m_Stage1FilterA1.Compress(nA);
		nA = m_Stage1FilterA2.Compress(nA);
		nA = m_Stage1FilterA3.Compress(nA);

		nB = m_Stage1FilterB1.Compress(nB);
		nB = m_Stage1FilterB2.Compress(nB);
		nB = m_Stage1FilterB3.Compress(nB);
		return;
//*/

		nA = m_Stage1FilterA1.Compress(nA);
		nA = m_Stage1FilterA2.Compress(nA);
//		nA = m_Stage1FilterA3.Compress(nA);

		nB = m_Stage1FilterB1.Compress(nB);
		nB = m_Stage1FilterB2.Compress(nB);

		//int nTemp = nA; nA = nB; nB = nTemp;
//		nB = m_Stage1FilterB3.Compress(nB);

//		nA = nA - nB;
//		nB = nB + (nA / 2);


//		return;

		m_rbInputA[0] = nA; m_rbInputB[0] = nB;

		{
			int nPrediction1 = m_rbInputA[-1];
			int nPrediction2 = m_rbInputA[-2];
			int nPrediction3 = m_rbInputA[-1] - m_rbInputA[-2];
			int nPrediction4 = m_rbInputB[-1];

			int nTotalPrediction = (nPrediction1 * m_aryMA[0]) + (nPrediction2 * m_aryMA[1])
				+ (nPrediction3  * m_aryMA[2]) + (nPrediction4 * m_aryMA[3]);
			int nOutput = nA - (nTotalPrediction >> 13);

			if (nOutput > 0)
			{
				m_aryMA[0] -= 2*((nPrediction1) ? ((nPrediction1 >> 30) & 2) - 1 : 0);
				m_aryMA[1] -= (nPrediction2) ? ((nPrediction2 >> 30) & 2) - 1 : 0;
				m_aryMA[2] -= (nPrediction3) ? ((nPrediction3 >> 30) & 2) - 1 : 0;
				m_aryMA[3] -= 1*((nPrediction4) ? ((nPrediction4 >> 30) & 2) - 1 : 0);
			}
			else if (nOutput < 0)
			{
				m_aryMA[0] += 2*((nPrediction1) ? ((nPrediction1 >> 30) & 2) - 1 : 0);
				m_aryMA[1] += (nPrediction2) ? ((nPrediction2 >> 30) & 2) - 1 : 0;
				m_aryMA[2] += (nPrediction3) ? ((nPrediction3 >> 30) & 2) - 1 : 0;
				m_aryMA[3] += 1*((nPrediction4) ? ((nPrediction4 >> 30) & 2) - 1 : 0);
			}

			nA = nOutput;
		}
		{
			int nPrediction1 = m_rbInputB[-1];
			int nPrediction2 = m_rbInputB[-2];
			int nPrediction3 = 0;//m_rbInputB[-1] - m_rbInputB[-2];
			int nPrediction4 = m_rbInputA[0];

			int nTotalPrediction = (nPrediction1 * m_aryMB[0]) + (nPrediction2 * m_aryMB[1])
				+ (nPrediction3  * m_aryMB[2]) + (nPrediction4 * m_aryMB[3]);
			int nOutput = nB - (nTotalPrediction >> 13);

			if (nOutput > 0)
			{
				m_aryMB[0] -= 2*((nPrediction1) ? ((nPrediction1 >> 30) & 2) - 1 : 0);
				m_aryMB[1] -= (nPrediction2) ? ((nPrediction2 >> 30) & 2) - 1 : 0;
				m_aryMB[2] -= (nPrediction3) ? ((nPrediction3 >> 30) & 2) - 1 : 0;
				m_aryMB[3] -= 1*((nPrediction4) ? ((nPrediction4 >> 30) & 2) - 1 : 0);
			}
			else if (nOutput < 0)
			{
				m_aryMB[0] += 2*((nPrediction1) ? ((nPrediction1 >> 30) & 2) - 1 : 0);
				m_aryMB[1] += (nPrediction2) ? ((nPrediction2 >> 30) & 2) - 1 : 0;
				m_aryMB[2] += (nPrediction3) ? ((nPrediction3 >> 30) & 2) - 1 : 0;
				m_aryMB[3] += 1*((nPrediction4) ? ((nPrediction4 >> 30) & 2) - 1 : 0);
			}

			nB = nOutput;
		}


		m_rbInputA.IncrementSafe();
		m_rbInputB.IncrementSafe();


/*
//		nInput = m_Filter1.Compress(nInput);

		m_rbInput[0] = nInput;
		
		int nPrediction1 = m_rbInput[-1];
		int nPrediction2 = (2 * m_rbInput[-1]) - m_rbInput[-2];
		int nPrediction3 = m_rbInput[-1] - m_rbInput[-2];
		int nPrediction4 = m_nLastOutput;

		int nTotalPrediction = ((nPrediction1) * m_aryM[0]) + (nPrediction2 * m_aryM[1])
			+ ((nPrediction3 >> 1) * m_aryM[2]) + (nPrediction4 * m_aryM[3]);
		int nOutput = nInput - (nTotalPrediction >> 13);

		if (nOutput > 0)
		{
			m_aryM[0] -= (nPrediction1) ? ((nPrediction1 >> 30) & 2) - 1 : 0;
			m_aryM[1] -= (nPrediction2) ? ((nPrediction2 >> 30) & 2) - 1 : 0;
			m_aryM[2] -= (nPrediction3) ? ((nPrediction3 >> 30) & 2) - 1 : 0;
			m_aryM[3] -= (nPrediction4) ? ((nPrediction4 >> 30) & 2) - 1 : 0;
		}
		else if (nOutput < 0)
		{
			m_aryM[0] += (nPrediction1) ? ((nPrediction1 >> 30) & 2) - 1 : 0;
			m_aryM[1] += (nPrediction2) ? ((nPrediction2 >> 30) & 2) - 1 : 0;
			m_aryM[2] += (nPrediction3) ? ((nPrediction3 >> 30) & 2) - 1 : 0;
			m_aryM[3] += (nPrediction4) ? ((nPrediction4 >> 30) & 2) - 1 : 0;
		}

		m_nLastOutput = nOutput;
		m_rbInput.IncrementSafe();

		return nOutput;
		//*/
	}

protected:

	CScaledFirstOrderFilter<31, 5> m_Stage1FilterA1;
	CScaledFirstOrderFilter<24, 5> m_Stage1FilterA2;
	CScaledFirstOrderFilter<7, 5> m_Stage1FilterA3;

	CScaledFirstOrderFilter<31, 5> m_Stage1FilterB1;
	CScaledFirstOrderFilter<24, 5> m_Stage1FilterB2;
	CScaledFirstOrderFilter<7, 5> m_Stage1FilterB3;

	CRollBufferFast<int, 256, 4> m_rbInputA;
	CRollBufferFast<int, 256, 4> m_rbInputB;
	int m_aryMA[8]; int m_aryMB[8];
};

#endif // #ifndef APE_START_FILTER_H
