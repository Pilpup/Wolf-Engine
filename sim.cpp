#include <iostream>
#include <vector>

int main() {
    int width = 120;
    int height = 80;
    std::vector<std::vector<int>> grid(height, std::vector<int>(width, 0));

    int drawEndY = 70;
    float shearX = 1.5f;
    float scaleY = 0.4f;

    for (int x = 10; x < 20; x++) {
        for (int y = 10; y < 60; y++) {
            int h = drawEndY - y;
            if (h < 0) h = 0;

            int shadowX = x + (int)(h * shearX);
            int shadowY = drawEndY - (int)(h * scaleY);

            if (shadowX >= 0 && shadowX < width && shadowY >= 0 && shadowY < height) {
                grid[shadowY][shadowX]++;
            }
        }
    }

    for (int y = 46; y <= 55; y++) {
        std::cout << y << ": ";
        for (int x = 20; x < width; x++) {
            if(grid[y][x] == 0) std::cout << ".";
            else std::cout << grid[y][x];
        }
        std::cout << "\n";
    }
    return 0;
}
