from PIL import Image

imagem = Image.open("testes\images.jpg")


imagem = imagem.resize((64, 64))
imagem = imagem.convert("1") 

bitmap = imagem.tobytes()

print(f"Tamanho: {len(bitmap)} bytes")

imagem.show()

print("const uint8_t imagem[512] = {")
for i, b in enumerate(bitmap):
    print(f"0x{b:02X}, ", end="")
    if (i + 1) % 16 == 0:
        print()
print("};")