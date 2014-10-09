sampler s0 : register(s0);
float4 p0 : register(c0);

#define height (p0[1])

float4 main(float2 tex : TEXCOORD0) : COLOR
{
	// original pixel
	float4 c0 = tex2D(s0,tex);

	// uncomment to activate for HD only
	/*
	if(height > 719)
	{
		return c0;
	}
	*/

	// r=c0[0], g=c0[1], b=c0[2]
	// RGB [16,235] to YUV: 601 mode (128 is not added to Cb and Cr)
	float y=0.299*c0[0] + 0.587*c0[1] + 0.114*c0[2];
	float Cb=-0.172*c0[0] -0.339*c0[1] +0.511*c0[2];
	float Cr=0.511*c0[0] -0.428*c0[1] -0.083*c0[2];

	// YUV to RGB [16,235]: 709 mode (Cb and Cr are 128 less)
	float r=y+1.540*Cr;
	float g=y-0.459*Cr-0.183*Cb;
	float b=y+1.816*Cb;

	return float4(r,g,b,0);
}
