#ifndef INPUTSYSTEM_HPP
#define INPUTSYSTEM_HPP

class InputSystem
{
public:
    InputSystem();

    void KeyDown(unsigned int);
    void KeyUp(unsigned int);
    void SetMousePos(unsigned short x, unsigned short y) const;
    bool IsKeyDown(unsigned int) const;
    void GetMousePos(unsigned short *x, unsigned short *y) const;
    void MousePressed(unsigned int button);
    void MouseReleased(unsigned int button);
    bool IsMousePressed(unsigned int button) const;

private:
    bool m_Keys[256];
    unsigned int m_Mouse;

};

extern InputSystem* input;

#endif // INPUTSYSTEM_HPP
