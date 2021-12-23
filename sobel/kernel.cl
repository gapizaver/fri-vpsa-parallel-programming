int getPixel(__global unsigned char *image, int y, int x, int width, int height)
{
    if (x < 0 || x >= width)
        return 0;
    if (y < 0 || y >= height)
        return 0;
    return image[y * width + x];
}

__kernel void sobel(__global unsigned char *image_in, __global unsigned char *image_out, int width, int height)
{
    int i = get_global_id(1);
    int j = get_global_id(0);
    int Gx, Gy;
    int tempPixel;
    
    if (i < height && j < width) {
        //image_out[i * width + j] = image_in[i * width + j];
        //za vsak piksel v sliki
        Gx = -getPixel(image_in, i - 1, j - 1, width, height) - 2 * getPixel(image_in, i - 1, j, width, height) -
                getPixel(image_in, i - 1, j + 1, width, height) + getPixel(image_in, i + 1, j - 1, width, height) +
                2 * getPixel(image_in, i + 1, j, width, height) + getPixel(image_in, i + 1, j + 1, width, height);
        Gy = -getPixel(image_in, i - 1, j - 1, width, height) - 2 * getPixel(image_in, i, j - 1, width, height) -
                getPixel(image_in, i + 1, j - 1, width, height) + getPixel(image_in, i - 1, j + 1, width, height) +
                2 * getPixel(image_in, i, j + 1, width, height) + getPixel(image_in, i + 1, j + 1, width, height);
        tempPixel = sqrt((float)(Gx * Gx + Gy * Gy));
        if (tempPixel > 255)
            image_out[i * width + j] = 255;
        else
            image_out[i * width + j] = tempPixel;
    }
}
