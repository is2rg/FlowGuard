static int (*input_handler) (unsigned char c);

static void stdin_handle_fd(void)
{
    (*input_handler)((unsigned char)1);
}

int serial_line_input_byte(unsigned char c)
{
    return 42;
}

int main(void)
{
    input_handler = &serial_line_input_byte;
    stdin_handle_fd();
}
