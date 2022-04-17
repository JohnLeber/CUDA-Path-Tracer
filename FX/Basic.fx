//=============================================================================
// Basic.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Basic effect that currently supports transformations, lighting, and texturing.
//=============================================================================

#include "LightHelper.fx"
float4 gVertexLineColor;
 
cbuffer cbPerFrame
{
	DirectionalLight gDirLights[3];
	float3 gEyePosW;

	float  gFogStart;
	float  gFogRange;
	float4 gFogColor;
};

cbuffer cbPerObject
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
	float4x4 gTexTransform;
	Material gMaterial;
}; 

// Nonnumeric values cannot be added to a cbuffer.
Texture2D gDiffuseMap;

SamplerState samAnisotropic
{
	Filter = ANISOTROPIC;
	MaxAnisotropy = 4;

	AddressU = WRAP;
	AddressV = WRAP;
};
 
struct VertexIn
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
};

struct VertexInTex
{
	float3 PosL    : POSITION;
	float3 NormalL : NORMAL;
	float2 Tex     : TEXCOORD;
};

struct VertexOutTex
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 Tex     : TEXCOORD;
};

struct VertexLineIn
{
	float3 PosL  : POSITION; 
};
struct VertexLineOut
{
	float4 PosH  : SV_POSITION; 
};

VertexLineOut VSLine(VertexLineIn vin)
{
	VertexLineOut vout;

	// Transform to world space space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	 
	return vout;
}

float4 PSLine(VertexLineOut pin) : SV_Target
{
	return gVertexLineColor;
}

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to world space space.
	vout.PosW    = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);
		
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
	return vout;
}
 
float4 PS(VertexOut pin, uniform int gLightCount) : SV_Target
{
	// Interpolating normal can unnormalize it, so normalize it.
    pin.NormalW = normalize(pin.NormalW);

	// The toEye vector is used in lighting.
	float3 toEye = gEyePosW - pin.PosW;

	// Cache the distance to the eye from this surface point.
	float distToEye = length(toEye); 

	// Normalize.
	toEye /= distToEye;
	
	//
	// Lighting.
	//


	// Start with a sum of zero. 
	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec    = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// Sum the light contribution from each light source.  
	[unroll]
	for(int i = 0; i < gLightCount; ++i)
	{
		float4 A, D, S;
		ComputeDirectionalLight(gMaterial, gDirLights[i], pin.NormalW, toEye, A, D, S);

		ambient += A;
		diffuse += D;
		spec    += S;
	}

	float4 litColor = ambient + diffuse + spec;

	// Common to take alpha from diffuse material.
	//litColor.a = gMaterial.Diffuse.a;

	//litColor.a = gMaterial.Diffuse.a;
	//litColor.r = 1.0f;
	//litColor.g = 0.0f;
	//litColor.b = 1.0f;
    return litColor;
}

float4 PSSun(VertexOut pin) : SV_Target
{
	// Interpolating normal can unnormalize it, so normalize it.
	pin.NormalW = normalize(pin.NormalW);

	// The toEye vector is used in lighting.
	float3 toEye = gEyePosW - pin.PosW;

	// Cache the distance to the eye from this surface point.
	float distToEye = length(toEye);

	// Normalize.
	toEye /= distToEye;

	//
	// Lighting.
	//


	// Start with a sum of zero. 
	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(1.0f, 1.0f, 0.0f, 0.0f);
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);
	 
	float4 litColor = ambient + diffuse + spec;

	// Common to take alpha from diffuse material.
	litColor.a = gMaterial.Diffuse.a;

	litColor.a = 0.6f;// gMaterial.Diffuse.a;
	//litColor.r = 1.0f;
	//litColor.g = 0.0f;
	//litColor.b = 1.0f;
	return litColor;
}

VertexOutTex VSTex(VertexInTex vin)
{
	VertexOutTex vout;

	// Transform to world space space.
	vout.PosW = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);

	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

	// Output vertex attributes for interpolation across triangle.
	vout.Tex = mul(float4(vin.Tex, 0.0f, 1.0f), gTexTransform).xy;

	return vout;
}

float4 PSTex(VertexOutTex pin) : SV_Target
{
	// Interpolating normal can unnormalize it, so normalize it.
	pin.NormalW = normalize(pin.NormalW);

	// The toEye vector is used in lighting.
	float3 toEye = gEyePosW - pin.PosW;

	// Cache the distance to the eye from this surface point.
	float distToEye = length(toEye);

	// Normalize.
	toEye /= distToEye;

	// Default to multiplicative identity.
	float4 texColor = gDiffuseMap.Sample(samAnisotropic, pin.Tex);
 
	float4 litColor = texColor; 
	// Common to take alpha from diffuse material and texture.
	litColor.a = 1;// gMaterial.Diffuse.a* texColor.a;

	return litColor;
}
//-----------------------------------------------------------------------------//
struct VertexScreenSpaceIn
{
	float3 PosL    : POSITION;
};
//-----------------------------------------------------------------------------//
struct VertexScreenSpaceOut
{
	float4 posH : SV_POSITION;
};
//-----------------------------------------------------------------------------//
VertexScreenSpaceOut VSScreenSpace(VertexScreenSpaceIn vIn)
{
	VertexScreenSpaceOut outVS = (VertexScreenSpaceOut)0;
	outVS.posH = float4(vIn.PosL, 1.0f);
	return outVS;
}
//-----------------------------------------------------------------------------//
float4 PSScreenSpace() : SV_Target
{
	return gVertexLineColor;
}
//-----------------------------------------------------------------------------//
technique11 ScreenSpaceTech
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VSScreenSpace()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PSScreenSpace()));
		//vertexShader = compile vs_3_0 PathSS2VS();
		//pixelShader = compile ps_3_0 PathSS2PS();
		//ZEnable = false;
	//	SetDepthStencilState(NoDepthWrites, 0);
		SetRasterizerState(0);
	}
}
technique11 Lines
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VSLine()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PSLine()));
	}
}

technique11 Slider
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VSTex()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PSTex()));
	}
}

technique11 Sun
{
	pass P0
	{
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PSSun()));
	}
}

technique11 Light1
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(1) ) );
    }
}

technique11 Light2
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(2) ) );
    }
}

technique11 Light3
{
    pass P0
    {
        SetVertexShader( CompileShader( vs_5_0, VS() ) );
		SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, PS(3) ) );
    }
}
