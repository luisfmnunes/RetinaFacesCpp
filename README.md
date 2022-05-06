# RetinaFace C++

This is the C++ implementation of the [Retinaface PyTorch](https://github.com/biubug6/Pytorch_Retinaface) model inference.

The Inference Engine used is the [ONNX Runtime](https://onnxruntime.ai/) Framework, and the model is obtained through the PyTorch ONNX export API. A pretrained model is provided in [this path](model/retinaface_dynamic.zip) with dynamic input shape. In the current implementation, images with any dimension above 640px are resized to 640x640 letterbox padding, due to anchors decoding limitation.

## Dependencies
- OpenCV 3 or higher

## Usage

Bellow are the usage instructions

### Unzip Pretrained Model

If there isn't an intent to provide a custom trained model, the first step consists in extracting the [pretrained model](model/retinaface_dynamic.zip). The following command called from the root path of this repo unzips the model.

```
unzip model/retinaface_dynamic.zip
```

### Compiling the main example

If a custom trained model is intended to be used, modify the [main script](main.cpp) in order to provide the correct path for model loading.

Use the following instructions to compile the [main script](main.cpp):

```
mkdir build && cd build
cmake ..
```

### Testing on an image

If the compilation succeded a binary file called RetinaCpp will be generated into the build directory, which can be used to detect faces in an image using the following structure:

```
./RetinaCpp PATH_TO_IMAGE
```

where **PATH_TO_IMAGE** should be replaced with a path to an image file.

## Results

Bellow there is an example of a detection result provided by this current implementation.

<image align="center" src="det.jpg">


