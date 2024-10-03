#version 300 es
precision highp float;
out vec4 FragColor;

in vec2 texcoord;

uniform sampler2D tex;
uniform vec3 col;
uniform float a;
uniform float b;
uniform float c;
uniform float d;

uniform mat4 modelMat;
uniform float aabbl;
uniform float aabbr;
uniform float aabbb;
uniform float aabbt;
uniform float ratio;
uniform int aaRes;
uniform vec2 resolution;
uniform mat3 trans;
uniform int binarySearchIterations;
uniform bool combineMosaic;
uniform int combineMode;
uniform bool showTransform;
uniform ivec2 grid;
uniform int gridNumber;
uniform float asdasd1;
uniform float asdasd2;

float lensDistortion(float r, float a, float b, float c, float d) {
	return (a * r * r + b * r + c) * r * r + d * r;//6 multiplications
}
float inverseLerp(float a, float b, float t) {
	return (t - a) / (b - a);
}
float inverseLensDistortion(float r, float a, float b, float c, float d) {
	if (a == 0. && b == 0. && c == 0. && d == 1.) return r;
	float answer = 0.;

	float start = 0.;
	float end = 10.;

	for (int i = 0; i < binarySearchIterations; i++) {
		answer = (start + end) * 0.5;

		if (lensDistortion(answer, a, b, c, d) < r) {
			start = answer;
		} else {
			end = answer;
		}
	}

	return answer;
}

const int howman = 48;
const int howmanycolors = 3;
vec3[howmanycolors] colors = vec3[](
	vec3(0.09, 0.19, 0.32), // blue
	vec3(0.50, 0.50, 0.50), // gray
	vec3(0.15, 0.06, 0.12) // red
);
/*vec3[](
	vec3(0.55, 0.53, 0.51), // dark white
	vec3(0.47, 0.39, 0.35), // brown
	vec3(0.62, 0.63, 0.64), // white
	vec3(0.31, 0.40, 0.48), // blue
	vec3(0.00, 0.08, 0.20) // black
);*/
const vec3[howmanycolors] palette = vec3[](
	vec3(0.09, 0.19, 0.32), // blue
	vec3(0.50, 0.50, 0.50), // gray
	vec3(0.15, 0.06, 0.12) // red
);/*vec3[](
	vec3(0.85, 0.78, 0.71), // dark white
	vec3(0.83, 0.69, 0.58), // brown
	vec3(0.88, 0.83, 0.78), // white
	vec3(0.69, 0.71, 0.69), // blue
	vec3(0.60, 0.61, 0.58) // black
);*/
int peartition(inout float[howman] arr, int l, int r) {
	int pivot = (r - l + 1) / 2;
	float temp = arr[l + pivot];
	arr[l + pivot] = arr[r];
	arr[r] = temp;

	float lst = arr[r];
	int i = l, j = l;
	while (j < r) {
		if (arr[j] < lst) {
		    temp = arr[i];
		    arr[i] = arr[j];
		    arr[j] = temp;
			i++;
		}
		j++;
	}
	temp = arr[i];
	arr[i] = arr[r];
	arr[r] = temp;
	return i;
}

// Utility function to find median
void median(float[howman] arr, int l, int r, int k, inout float a, inout float b) {
    for (int i = 0; i < 100; i++) {
    	if (l <= r) {
    		int partitionIndex = peartition(arr, l, r);

    		if (partitionIndex == k) {
    			b = arr[partitionIndex];
    			if (a != -1.) return;
    		} else if (partitionIndex == k - 1) {
    			a = arr[partitionIndex];
    			if (b != -1.) return;
    		}

    		if (partitionIndex >= k)
    		    r = partitionIndex - 1;
    		else
    		    l = partitionIndex + 1;
    	}
    }
}

// Function to find Median
float getMedian(float[howman] arr, int n) {
	if (n == 0) return 0.;
	float a = -1., b = -1.;

	median(arr, 0, n - 1, n / 2, a, b);
	return (n % 2 == 1) ? b : (a + b) * 0.5;
}

float modRange(float x, float a, float b) {
	return mod(x - a, b - a) + a;
}
vec4 doPixel(vec2 uv) {
	//uv = vec2(mix(mix(perspA.x, perspB.x, uv.y), mix(perspD.x, perspC.x, uv.y), uv.x), mix(mix(perspA.y, perspD.y, uv.x), mix(perspB.y, perspC.y, uv.x), uv.y));
	//uv = vec2(inverseLerp(mix(perspA.x, perspB.x, uv.y), mix(perspD.x, perspC.x, uv.y), uv.x), inverseLerp(mix(perspA.y, perspD.y, uv.x), mix(perspB.y, perspC.y, uv.x), uv.y));

	if (showTransform) {
		vec3 transformed = vec3(uv, 1.);
		transformed = trans * transformed;
		uv = (transformed.xy) / transformed.z;
	}

	uv = uv * 2. - 1.;
	uv.x *= ratio;
	float r = length(uv);
	r = inverseLensDistortion(r, a, b, c, d);
	uv = normalize(uv) * r;

	uv.x /= ratio;
	uv = (uv + 1.) * 0.5;

	return texture(tex, uv);
}
vec4 doGridPixel(vec2 uv, int ecks, int why) {
		/*uv.x = mix(float(ecks) / float(grid.x), float(ecks + 1) / float(grid.x), aabbl),
		mix(float(ecks) / float(grid.x), float(ecks + 1) / float(grid.x), aabbr),
		mix(float(why) / float(grid.y), float(why + 1) / float(grid.y), aabbb),
		mix(float(why) / float(grid.y), float(why + 1) / float(grid.y), aabbt)*/
	uv.x = mix(float(ecks) / float(grid.x), float(ecks + 1) / float(grid.x), uv.x);
	uv.y = mix(float(why) / float(grid.y), float(why + 1) / float(grid.y), uv.y);
	return doPixel(uv);
}

void main() {
	float[howman] red, green, blue;
	float w = 1. / resolution.x / float(aaRes);
	float h = 1. / resolution.y / float(aaRes);
	vec4 color = vec4(0.);
	//aa
	for (int aa = 0; aa < aaRes * aaRes; aa++) {
		vec2 youvee = texcoord + vec2(w * float(aa % aaRes), h * float(aa / aaRes));
		vec2 uv = vec2(mix(aabbl, aabbr, texcoord.x), mix(aabbb, aabbt, texcoord.y));
		if (combineMosaic) uv = mod(uv, 1.);

		if (combineMosaic) {
			float howmany = 0.;
			int howmanyMedian = 0;
			vec3 currentColor = vec3(0.);

			if (combineMode == 0) {
				for (int i = 0; i < grid.x * grid.y; i++) {
					vec4 pixelColor = doGridPixel(uv, i % grid.x, i / grid.x);
					currentColor += pixelColor.rgb * pixelColor.a;
					howmany += pixelColor.a;
				}
				currentColor /= howmany;
			} else if (combineMode == 1) {
				for (int i = 0; i < grid.x * grid.y; i++) {
					vec4 pixelColor = doGridPixel(uv, i % grid.x, i / grid.x);
					if (pixelColor.a > 0.5) {
						red[howmanyMedian] = pixelColor.r;
						green[howmanyMedian] = pixelColor.g;
						blue[howmanyMedian] = pixelColor.b;
						howmanyMedian++;
					}
				}
				currentColor.r = getMedian(red, howmanyMedian);
				currentColor.g = getMedian(green, howmanyMedian);
				currentColor.b = getMedian(blue, howmanyMedian);
			} else if (combineMode == 2) { // single
				currentColor = doGridPixel(uv, gridNumber % grid.x, gridNumber / grid.x).rgb;
			} else if (combineMode == 3) {
				int[howmanycolors] currentColors = int[](0,0,0);
				for (int i = 0; i < grid.x * grid.y; i++) {
					vec4 pixelColor = doGridPixel(uv, i % grid.x, i / grid.x);

					//get closest color
					int closestColor = 0;
					float closestColorDistance = 1000.;
					for (int j = 0; j < howmanycolors; j++) {
						float dist = distance(pixelColor.rgb, colors[j]);
						if (dist < closestColorDistance) {
							closestColorDistance = dist;
							closestColor = j;
						}
					}
					currentColors[closestColor]++;
				}
				//get mode
				vec3 mostColor = vec3(0.);
				int mostColorCount = 0;
				for (int i = 0; i < howmanycolors; i++) {
					if (currentColors[i] > mostColorCount) {
						mostColor = palette[i];
						mostColorCount = currentColors[i];
					}
				}
				currentColor.rgb = mostColor;

			}
			color += vec4(currentColor, 1.);
		} else {
			color += doPixel(uv);
		}
	}
	color /= float(aaRes * aaRes);
	color.rgb *= col;

	FragColor = color;
}
