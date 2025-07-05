# Angry Birds Oyunu - Nasıl Oynanır

## Oyunu Başlatma

### Yöntem 1: Basit Başlatma
```
start.bat
```
Çift tıklayın veya terminal'de yazın.

### Yöntem 2: Detaylı Başlatma
```
run_game.bat
```
Kontrolleri gösterir ve oyunu başlatır.

### Yöntem 3: PowerShell ile (Renkli)
```
./run_game.ps1
```
PowerShell'de renkli çıktı ile oyunu başlatır.

## Oyun Kontrolleri

- **Kuşu Fırlatma**: Kuşa tıklayın, sürükleyin ve bırakın
- **Oyunu Resetleme**: R tuşuna basın
- **Oyundan Çıkma**: Pencereyi kapatın

## Oyun Amacı

1. Kuşu sapandan fırlatarak blokları ve düşmanları vurun
2. Tüm düşmanları öldürün
3. 3 canınız var, dikkatli kullanın!

## Sorun Giderme

- Eğer oyun açılmıyorsa, `x64/Debug/projee.exe` dosyasının var olduğundan emin olun
- Script çalışmıyorsa, dosyaları sağ tıklayıp "Yönetici olarak çalıştır" deneyin

## Geliştirici Notları

- C dili ve Raylib kütüphanesi kullanılarak geliştirilmiştir
- Kaynak kod: `main.c`
- Proje dosyaları: `projee.sln`, `projee.vcxproj` 