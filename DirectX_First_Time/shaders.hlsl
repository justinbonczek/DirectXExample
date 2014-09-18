
// Output of Vertex Shader
struct VOut
{
    float4 position : SV_POSITION;
    float4 color : COLOR;
};

// Vertex Shader
VOut main(float4 position : POSITION, float4 color : COLOR)
{
	VOut output;

	output.position = position;
	output.color = color;

	return output;
}

// Pixel Shader
float4 PShader(float4 position : SV_POSITION, float4 color : COLOR) : SV_TARGET
{
	return color;
}