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
float inverseLensDistortion(float distortedR, float a, float b, float c, float d) {
    float r = distortedR;

    for (int i = 0; i < binarySearchIterations; ++i) { // 10 iterations should be sufficient
        float f = lensDistortion(r, a, b, c, d) - distortedR;
        float derivative = r * r * (4.0 * a * r + 3.0 * b) + 2.0 * c * r + d;

        if (abs(derivative) < 1e-6) {
            break;
        }

        r -= f / derivative;
    }

    return r;
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

	if (showTransform) {
		vec3 transformed = vec3(uv, 1.);
		transformed = trans * transformed;
		uv = transformed.xy / transformed.z;
	}

	uv = uv * 2. - 1.;
	uv.x *= ratio;
	float r = length(uv);
	r = inverseLensDistortion(r, a, b, c, d);
	uv = normalize(uv) * r;

	uv.x /= ratio;
	uv = (uv + 1.) * 0.5;

	vec4 texel = texture(tex, uv);
	texel.a = texel.a < 0.5 ? 0. : 1.;
	return texel;
}
vec4 doGridPixel(vec2 uv, int ecks, int why) {
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
			vec3 currentColor = vec3(0.);

			switch (combineMode) {
			case 0:{ // mean
				float howmany = 0.;
				for (int i = 0; i < grid.x * grid.y; i++) {
					vec4 pixelColor = doGridPixel(uv, i % grid.x, i / grid.x);
					currentColor += pixelColor.rgb * pixelColor.a;
					howmany += pixelColor.a;
				}
				currentColor /= howmany;
				break;
			}
			case 1:{ // median
				int howmanyMedian = 0;
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
				break;
			}
			case 2:{ // single
				currentColor = doGridPixel(uv, gridNumber % grid.x, gridNumber / grid.x).rgb;
				break;
			}
			case 3:{ // color palette
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
				currentColor = mostColor;
				break;
			}
			case 4:{ // mad
				const float multiplier = 10.;

				float howmany = 0.;
				// get mean
				vec3 mean = vec3(0.);
				for (int i = 0; i < grid.x * grid.y; i++) {
					vec4 pixelColor = doGridPixel(uv, i % grid.x, i / grid.x);
					mean += pixelColor.rgb * pixelColor.a;
					howmany += pixelColor.a;
				}
				mean /= howmany;

				howmany = 0.;
				// get mad
				vec3 mad = vec3(0.);
				for (int i = 0; i < grid.x * grid.y; i++) {
					vec4 pixelColor = doGridPixel(uv, i % grid.x, i / grid.x);
					mad += abs(pixelColor.rgb - mean) * multiplier * pixelColor.a; // absolute deviation
					howmany += pixelColor.a;
				}
				mad /= howmany;
				currentColor = mad;
				break;
			}
			}
			color += vec4(currentColor, 1.);
		} else { // not combine mosaic
			color += doPixel(uv);
		}
	}
	color /= float(aaRes * aaRes);
	color.rgb *= col;

	FragColor = color;
}
