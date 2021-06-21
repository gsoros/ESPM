R"===========(
<html>

<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESPM</title>
  <style>
    canvas {
      width: 100%;
      height: 75%;
    }

    body {
      margin: 0;
      background: #000;
    }
  </style>
  <script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
  <script>
    function request(path, onDone) {
      var xhr = new XMLHttpRequest();
      xhr.open("GET", path, true);
      xhr.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          onDone(this.responseText);
        };
      };
      xhr.send();
      return xhr.responseText;
    }

    var scene;
    var camera;
    var renderer;
    var cube;

    function setupScene() {
      scene = new THREE.Scene();
      scene.add(new THREE.AmbientLight(0xffffff, .3));
      var light = new THREE.PointLight(0xffffff, .8);
      light.position.set(120, 120, 120);
      scene.add(light);
      camera = new THREE.PerspectiveCamera(
        75,
        window.innerWidth / window.innerHeight,
        0.1,
        1000
      );
      camera.position.z = 200;
      var geometry = new THREE.BoxGeometry(18, 150, 2);
      var material = new THREE.MeshPhongMaterial({ color: 0x000000 });
      cube = new THREE.Mesh(geometry, material);
      scene.add(cube);
      renderer = new THREE.WebGLRenderer();
      renderer.setSize(window.innerWidth, window.innerHeight * .75);
      document.getElementById("animation").appendChild(renderer.domElement);
      renderer.render(scene, camera);
    }
    function webSocketBegin() {
      if (!"WebSocket" in window) {
        console.log("ws unsupported");
        return;
      }
      ws = new WebSocket("ws://" + location.hostname + ":8001/");

      ws.onopen = function () {
        console.log("ws open");
      };

      ws.onmessage = function (evt) {
        //console.log(evt);
        var j = JSON.parse(evt.data);
        cube.quaternion.set(j.qX, -j.qY, -j.qZ, j.qW);
        cube.material.color.setRGB(.3 + j.strain / 200, .3, .3 - j.strain / 200);
        renderer.render(scene, camera);
      };

      ws.onclose = function () {
        console.log("ws close");
        webSocketBegin();
      };

    }
  </script>
</head>

<body onLoad="javascript: setupScene(); webSocketBegin()">
  <div id="animation"></div>
  <button
    onclick="if (confirm('Device should be motionless')) request('/calibrateAccelGyro', function(response) {alert(response)})">
    Calibrate A/G
  </button>
  <button
    onclick="if (confirm('Wave device around gently for 15 seconds')) request('/calibrateMag', function(response) {alert(response)})">
    Calibrate Mag
  </button>
  <button onclick="if (confirm('Reboot ESP?')) request('/reboot'); setTimeout(window.location.reload(), 5000)">
    Reboot
  </button>
</body>

</html>

)==========="