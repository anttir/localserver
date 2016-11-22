var scene; // Global scene object
var camera; // Global camera object
var cubeMesh; // Global mesh object of the cube

$(function () {
    // Initialize the scene
    initializeScene();

    //// Animate the scene
    //animateScene();
})
/**
 * Initialze the scene.
 */
function initializeScene() {
    // Check whether the browser supports WebGL. If so, instantiate the hardware accelerated
    // WebGL renderer. For antialiasing, we have to enable it. The canvas renderer uses
    // antialiasing by default.
    // The approach of multiplse renderers is quite nice, because your scene can also be
    // viewed in browsers, which don't support WebGL. The limitations of the canvas renderer
    // in contrast to the WebGL renderer will be explained in the tutorials, when there is a
    // difference.
    renderer = new THREE.WebGLRenderer({ antialias: true });
    //if (Detector.webgl) {
    //    renderer = new THREE.WebGLRenderer({ antialias: true });
    //    // If its not supported, instantiate the canvas renderer to support all non WebGL browsers
    //} else {
    //    renderer = new THREE.CanvasRenderer();
    //}

    // Set the background color of the renderer to black, with full opacity
    //renderer.setClearColor(0x000000, 1);



    // Get the DIV element from the HTML document by its ID and append the renderers DOM
    // object to it
    var canvas =  document.getElementById("WebGLCanvas");
    canvas.appendChild(renderer.domElement);
    // Get the size of the inner window (content area) to create a full size renderer
    canvasWidth = canvas.clientWidth;
    canvasHeight = canvas.clientHeight;

    // Set the renderers size to the content areas size
    renderer.setSize(canvasWidth, canvasHeight);
    // Create the scene, in which all objects are stored (e. g. camera, lights,
    // geometries, ...)
    scene = new THREE.Scene();

    // Now that we have a scene, we want to look into it. Therefore we need a camera.
    // Three.js offers three camera types:
    //  - PerspectiveCamera (perspective projection)
    //  - OrthographicCamera (parallel projection)
    //  - CombinedCamera (allows to switch between perspective / parallel projection
    //    during runtime)
    // In this example we create a perspective camera. Parameters for the perspective
    // camera are ...
    // ... field of view (FOV),
    // ... aspect ratio (usually set to the quotient of canvas width to canvas height)
    // ... near and
    // ... far.
    // Near and far define the cliping planes of the view frustum. Three.js provides an
    // example (http://mrdoob.github.com/three.js/examples/
    // -> canvas_camera_orthographic2.html), which allows to play around with these
    // parameters.
    // The camera is moved 10 units towards the z axis to allow looking to the center of
    // the scene.
    // After definition, the camera has to be added to the scene.
    camera = new THREE.PerspectiveCamera(45, canvasWidth / canvasHeight, 1, 100);
    camera.position.set(0, 0, 10);
    camera.lookAt(scene.position);
    scene.add(camera);

    // Create the cube
    // Parameter 1: Width
    // Parameter 2: Height
    // Parameter 3: Depth
    var boxGeometry = new THREE.BoxGeometry(1.5, 1.5, 1.5);

    // Applying different materials to the faces is a more difficult than applying one
    // material to the whole geometry. We start with creating an array of
    // THREE.MeshBasicMaterial.

    // Define six colored materials
    var boxMaterials = [
        new THREE.MeshBasicMaterial({ color: 0xFF0000 }),
        new THREE.MeshBasicMaterial({ color: 0x00FF00 }),
        new THREE.MeshBasicMaterial({ color: 0x0000FF }),
        new THREE.MeshBasicMaterial({ color: 0xFFFF00 }),
        new THREE.MeshBasicMaterial({ color: 0x00FFFF }),
        new THREE.MeshBasicMaterial({ color: 0xFFFFFF })
    ];

    // Create a MeshFaceMaterial, which allows the cube to have different materials on
    // each face
    var boxMaterial = new THREE.MeshFaceMaterial(boxMaterials);

    // Create a mesh and insert the geometry and the material. Translate the whole mesh
    // by 1.5 on the x axis and by 4 on the z axis and add the mesh to the scene.
    boxMesh = new THREE.Mesh(boxGeometry, boxMaterial);
    //boxMesh.position.set(1.5, 0.0, 4.0);
    scene.add(boxMesh);
}


function rotateCube(ax,ay,az) {
    boxMesh.rotation.x = ax * Math.PI / 180;
    boxMesh.rotation.y = ay * Math.PI / 180;
    boxMesh.rotation.z = az * Math.PI / 180;
    // Map the 3D scene down to the 2D screen (render the frame)
    renderScene();
}

/**
 * Render the scene. Map the 3D world to the 2D screen.
 */
function renderScene() {
    renderer.render(scene, camera);
}