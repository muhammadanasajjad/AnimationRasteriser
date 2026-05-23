class Renderer {
    public:
        void render();
        void load();
        void offload();
    private:
        int frame = 0;
        unsigned int vertexShader;
        unsigned int fragmentShader;
        unsigned int mainShaderProgram;
        unsigned int VAO, VBO;
};
