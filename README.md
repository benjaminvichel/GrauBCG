# 🎓 GrauBCG - Atividades de Computação Gráfica

Projeto com múltiplos módulos desenvolvidos usando OpenGL moderno (GLFW, GLAD, GLM, stb_image, shaders GLSL 450). Inclui também um trabalho final com animação, iluminação Phong e interações com teclado/mouse.

---

## 🔧 Como Executar

```bash
cmake -B build
cd build
./Modulo2.exe

--------------------------------------//---------------------------------------------------
Módulo 1: 
ESC: Fecha a janela.

X: Ativa rotação em torno do eixo X.

Y: Ativa rotação em torno do eixo Y.

Z: Ativa rotação em torno do eixo Z.
--------------------------------------//---------------------------------------------------
Módulo 2:
ESC: Fecha a janela e encerra o programa.

X: Inicia a rotação dos cubos no eixo X.

Y: Inicia a rotação dos cubos no eixo Y.

Z: Inicia a rotação dos cubos no eixo Z.
--------------------------------------//---------------------------------------------------
Módulo 3:

X → Rotaciona o cubo em torno do eixo X.

Y → Rotaciona o cubo em torno do eixo Y.

Z → Rotaciona o cubo em torno do eixo Z.
--------------------------------------//---------------------------------------------------
Módulo 4:

ESC	Fecha a janela e encerra o programa
X	Inicia rotação contínua no eixo X
Y	Inicia rotação contínua no eixo Y
Z	Inicia rotação contínua no eixo Z
--------------------------------------//---------------------------------------------------
Módulo 5:
ESC	Fecha a janela e encerra o programa
shift/space para subir e descer
w/a/s/d para movimentar

Utilizar Mouse
--------------------------------------//---------------------------------------------------
Módulo 6:

ESC	Fecha a janela e encerra o programa
TAB	Alterna entre os objetos animados
E (Press)	Adiciona um waypoint à posição atual da câmera para o objeto ativo
E (Release)	Libera o bloqueio da tecla E para permitir novo waypoint
shift/space para subir e descer
w/a/s/d para movimentar
Mouse 
--------------------------------------//---------------------------------------------------
Trabalho Grau B:

Video de apresentação em: https://youtu.be/mtHLGS3dji0

ESC	Fecha o programa (fecha a janela)
TAB	Alterna entre os objetos animados (seleciona o próximo objeto)

E	Adiciona um waypoint na posição atual da câmera para o objeto selecionado (pressione e solte para adicionar um waypoint novo)

1	Diminui a intensidade da luz ambiente (ambientStrength - 0.1)
2	Aumenta a intensidade da luz ambiente (ambientStrength + 0.1)
3	Diminui a intensidade da luz difusa (diffuseStrength - 0.1)
4	Aumenta a intensidade da luz difusa (diffuseStrength + 0.1)
5	Diminui a intensidade da luz especular (specularStrength - 0.1)
6	Aumenta a intensidade da luz especular (specularStrength + 0.1)
R	Rotaciona o objeto selecionado 10 graus no eixo X
T	Rotaciona o objeto selecionado 10 graus no eixo Y
Y	Rotaciona o objeto selecionado 10 graus no eixo Z
U	Diminui a escala do objeto selecionado (mínimo 0.1)
I	Aumenta a escala do objeto selecionado
shift/space para subir e descer
w/a/s/d para movimentar
Mouse
