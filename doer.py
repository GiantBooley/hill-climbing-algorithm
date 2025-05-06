import numpy as np
import sys # For checking float precision

def calculate_rectified_aspect_ratio(points_quad, principal_point):
    """
    Calculates the aspect ratio of a rectangle after correcting perspective distortion
    using the vanishing point method.

    Args:
        points_quad (list or tuple): A list/tuple of 4 tuples, representing the
                                     (x, y) coordinates of the quadrilateral's
                                     corners in sequential order (e.g., clockwise
                                     or counter-clockwise).
                                     Example: [(x0, y0), (x1, y1), (x2, y2), (x3, y3)]
        principal_point (tuple): The (px, py) coordinates of the perspective
                                 principal point in the image.

    Returns:
        float: The calculated aspect ratio (width / height) of the rectified rectangle.
               Returns None if calculations fail (e.g., degenerate input).
    """
    if len(points_quad) != 4:
        raise ValueError("Input must contain exactly 4 points.")

    # --- 0. Prepare Points (Homogeneous Coordinates) ---
    # Convert points to numpy array and use homogeneous coordinates (x, y, 1)
    # It's often helpful to center the coordinates around the principal point
    # for perspective calculations, although it might not strictly be necessary
    # for *this specific* aspect ratio calculation via VPs.
    # We will proceed by centering as it's a common practice.
    
    pp = np.array(principal_point, dtype=float)
    pts = np.array(points_quad, dtype=float)
    
    # Optional: Center points around the principal point
    # pts_centered = pts - pp 
    # P_h = np.hstack((pts_centered, np.ones((4, 1)))) 
    
    # Using original coordinates directly in homogeneous form
    P_h = np.hstack((pts, np.ones((4, 1)))) 
    P0, P1, P2, P3 = P_h[0], P_h[1], P_h[2], P_h[3]

    # --- 1. Define Lines of the Quadrilateral Sides ---
    # Lines are represented in homogeneous coordinates by the cross product
    # of the two points defining them (L = P1 x P2).
    L01 = np.cross(P0, P1)
    L12 = np.cross(P1, P2)
    L23 = np.cross(P2, P3)
    L30 = np.cross(P3, P0)

    # --- 2. Find Vanishing Points ---
    # Vanishing points are the intersections of lines corresponding to
    # parallel sides in the real-world rectangle.
    # V1 = intersection of Line 01 and Line 23
    # V2 = intersection of Line 12 and Line 30
    
    # Normalize lines to avoid potential numerical issues (optional but good practice)
    L01 /= np.sqrt(L01[0]**2 + L01[1]**2)
    L12 /= np.sqrt(L12[0]**2 + L12[1]**2)
    L23 /= np.sqrt(L23[0]**2 + L23[1]**2)
    L30 /= np.sqrt(L30[0]**2 + L30[1]**2)

    V1 = np.cross(L01, L23)
    V2 = np.cross(L12, L30)

    # Check for near-zero homogenous coordinate (vanishing point at infinity - lines are parallel in image)
    # This would imply an affine view already, handle if needed, but for typical perspective it shouldn't be zero.
    if abs(V1[2]) < sys.float_info.epsilon or abs(V2[2]) < sys.float_info.epsilon:
         print("Warning: Potential parallel lines detected in the image. "
               "The input might be an affine transformation of a rectangle, "
               "or a degenerate case.")
         # We can try to proceed, but results might be unreliable.
         # Or return None here. Let's try to proceed for now.

    # --- 3. Find the Image of the Line at Infinity (Horizon Line) ---
    # The line connecting the two vanishing points.
    L_infinity = np.cross(V1, V2)
    
    # Normalize the line at infinity
    L_infinity_norm_factor = np.sqrt(L_infinity[0]**2 + L_infinity[1]**2 + L_infinity[2]**2)
    if L_infinity_norm_factor < sys.float_info.epsilon:
        print("Error: Vanishing points are degenerate (coincident or zero). Cannot compute horizon line.")
        return None
        
    L_infinity /= L_infinity_norm_factor


    # --- 4. Construct the Rectifying Homography ---
    # This homography maps the calculated L_infinity to the canonical line
    # at infinity (0, 0, 1). A common form for H_projective is:
    # H = [ 1  0  0 ]
    #     [ 0  1  0 ]
    #     [ l1 l2 l3]
    # where L_infinity = (l1, l2, l3). This removes the projective distortion.
    # Note: This H matrix corrects *only* the projective part. An affine
    #       transformation (scaling, rotation, shear, translation) might remain.

    H_projective = np.identity(3)
    H_projective[2, :] = L_infinity

    try:
        # We need the inverse mapping if applying to points, but for calculating
        # the *geometry* (aspect ratio), applying the forward H to the points
        # and measuring distances in the transformed space works.
        
        # Apply the homography to the original corner points
        P_transformed_h = (H_projective @ P_h.T).T # Note the transpose for multiplication

        # Convert back to Cartesian coordinates by dividing by the 3rd component
        # Handle cases where the third component might be zero or very small
        valid_pts = np.abs(P_transformed_h[:, 2]) > sys.float_info.epsilon
        if not np.all(valid_pts):
            print("Error: Point transformation resulted in points at infinity.")
            return None
            
        P_transformed = P_transformed_h[valid_pts, :2] / P_transformed_h[valid_pts, 2, np.newaxis]
        
        if P_transformed.shape[0] != 4:
             print("Error: Not all points transformed correctly.")
             return None
             
        PT0, PT1, PT2, PT3 = P_transformed

    except np.linalg.LinAlgError:
        print("Error: Matrix operation failed (potentially singular matrix).")
        return None

    # --- 5. Calculate Side Lengths of the Transformed Shape ---
    # The transformed points PT0, PT1, PT2, PT3 should now form a shape
    # that is an affine transformation of the original rectangle. We can
    # calculate the side lengths.
    width1 = np.linalg.norm(PT0 - PT1)
    height1 = np.linalg.norm(PT1 - PT2)
    width2 = np.linalg.norm(PT2 - PT3)
    height2 = np.linalg.norm(PT3 - PT0)

    # --- 6. Calculate Average Width and Height ---
    # Average the lengths of the opposite sides for robustness
    avg_width = (width1 + width2) / 2.0
    avg_height = (height1 + height2) / 2.0

    if avg_height < sys.float_info.epsilon:
        print("Error: Calculated height is close to zero.")
        return None

    # --- 7. Calculate Aspect Ratio ---
    aspect_ratio = avg_width / avg_height

    return aspect_ratio

# --- Example Usage ---
# Define the 4 corners of the quadrilateral in the image (clockwise/counter-clockwise)
# Example: A trapezoid representing a view of a square
#          Top edge shorter and farther away
quad_points = [
    (100, 100),  # Top-left
    (400, 120),  # Top-right
    (450, 400),  # Bottom-right
    (80, 380)    # Bottom-left
]

# Define the principal point (often the image center, but could be elsewhere)
# Example: Image size 600x500, principal point at center
image_width = 1920
image_height = 1080
principal_point = (image_width / 2, image_height / 2) # (300, 250)

# Calculate the aspect ratio
ratio = calculate_rectified_aspect_ratio(quad_points, principal_point)

if ratio is not None:
    print(f"Calculated Aspect Ratio (Width / Height): {ratio:.4f}")
    # For a square, the expected aspect ratio is 1.0
    # For a 16:9 wallpaper, it would be 16/9 = 1.777...
else:
    print("Could not calculate aspect ratio.")

# Example 2: A more distorted view
quad_points_distorted = [
    (751, 220),
    (1833, 502),
    (1198, 888),
    (63, 409)
]
principal_point_distorted = (image_width / 2, image_height / 2)

ratio_distorted = calculate_rectified_aspect_ratio(quad_points_distorted, principal_point_distorted)

if ratio_distorted is not None:
    print(f"\nDistorted Example Aspect Ratio (Width / Height): {ratio_distorted:.4f}")
else:
    print("\nCould not calculate aspect ratio for the distorted example.")