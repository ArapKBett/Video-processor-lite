// app.js
document.getElementById('processButton').addEventListener('click', () => {
    const videoInput = document.getElementById('videoInput').files[0];
    if (!videoInput) {
        alert('Please select a video file.');
        return;
    }

    const reader = new FileReader();
    reader.onload = function(event) {
        const videoData = new Uint8Array(event.target.result);
        const processVideo = Module.cwrap('process_video', 'string', ['array']);
        const result = processVideo(videoData);
        document.getElementById('output').innerText = result;
    };
    reader.readAsArrayBuffer(videoInput);
});
