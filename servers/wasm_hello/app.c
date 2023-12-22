void info(const char *str);

__attribute__((export_name("main")))
int main(void) {
    info("Hello WASM!");
    return 0;
}