# 📚 Fundamentos Matemáticos del Autotuning PID

Este documento complementa la documentación funcional del módulo de autotuning describiendo **el soporte teórico** de los dos métodos implementados en el proyecto:

1. **Ziegler–Nichols (Relay Feedback)**  
2. **Åström–Hägglund (Relay Feedback)**

Cada sección presenta los supuestos, el modelo simplificado, el procedimiento experimental y la deducción de las fórmulas usadas para obtener \(K_p, K_i, K_d\).

---

## 1. Ziegler–Nichols – Relay Feedback

### 1.1 Antecedentes

El método clásico de Ziegler y Nichols (1942) se diseñó para ajustar controladores PID en procesos industriales usando **la ganancia crítica** \(K_u\) y **el período crítico** \(P_u\).  
El enfoque *Relay Feedback* (Åström, Hägglund 1984) reemplaza la búsqueda de \(K_u\) contínua por un relé on/off que provoca oscilaciones sostenidas de amplitud definida.

### 1.2 Modelo Simplificado

Suponemos un **proceso lineal de primer orden con retardo** (FOPDT):
\[
    G(s) = \frac{K}{\tau s + 1} e^{-Ls}
\]
Para la deducción, basta que el sistema responda con oscilaciones aproximadamente sinusoidales bajo excitación de relé.

### 1.3 Magnitud del Relé

Se aplica al lazo cerrado un relé de amplitud \(d\) (diferencia entre niveles alto y bajo dividida por 2):
\[
    u(t) = \begin{cases}
        +d & \text{si } e(t) < 0 \\
        -d & \text{si } e(t) > 0
    \end{cases}
\]
La salida oscila con **amplitud** \(a\) y período \(P_u\).

### 1.4 Relacionando \(d\) y \(a\)

Para pequeñas no-linealidades el lazo se aproxima por la **descripción armónica**:
\[
    K_u = \frac{4d}{\pi a}
\]
*Derivación*: expandiendo el relé en serie de Fourier y considerando sólo el término fundamental.  
El primer armónico de la señal cuadrada tiene amplitud \(\tfrac{4d}{\pi}\), que se iguala a \(K_u a\) en régimen estacionario.

### 1.5 Fórmulas de Ziegler–Nichols (1952)

Una vez obtenidos \(K_u\) y \(P_u\), los parámetros para un controlador PID ideal son:
\[
\begin{aligned}
    K_p &= 0.6\,K_u \\
    K_i &= 2K_p / P_u = 1.2\,\frac{K_u}{P_u} \\
    K_d &= 0.075\,K_u\,P_u
\end{aligned}
\]
Estas constantes proporcionan un sobre-paso ≈ 25 % y velocidad de respuesta razonable para la mayoría de procesos de tipo FOPDT.

---

## 2. Åström–Hägglund – Relay Feedback

### 2.1 Motivación

Åström y Hägglund extendieron el método de relé para:
* Evitar operar el proceso al borde de la inestabilidad.  
* Generar automáticamente oscilaciones con un excitador no lineal sencillo.

### 2.2 Procedimiento Experimental

El algoritmo es idéntico en la práctica al usado en Z-N dentro de este proyecto:
1. Aplicar histéresis ± \(h\) alrededor del setpoint.  
2. Registrar temperaturas máxima y mínima para obtener amplitud \(a\).  
3. Medir tiempo entre flancos ascendentes (dos encendidos consecutivos) → \(P_u\).

### 2.3 Ganancia Crítica

Se reutiliza la deducción armónica, de modo que:
\[
    K_u = \frac{4d}{\pi a}
\]

### 2.4 Reglas de Sintonía

Åström & Hägglund presentan varias tablas.  Para un controlador PID **clásico** y comparación justa con Z-N conservamos la variante *PID closed-loop* (CL):
\[
\begin{aligned}
    K_p &= 0.6\,K_u \\
    T_i &= 0.5\,P_u \quad\Rightarrow\quad K_i = \frac{K_p}{T_i} = 1.2\,\frac{K_u}{P_u} \\
    T_d &= 0.12\,P_u \quad\Rightarrow\quad K_d = K_p T_d = 0.072\,K_u P_u
\end{aligned}
\]
> En esta implementación se usa \(0.075\,K_u P_u\) para \(K_d\) (valor intermedio entre varias recomendaciones), por compatibilidad con la regla de Z-N.

### 2.5 Ventajas respecto a Z-N

1. **Menor esfuerzo de excitación** – el sistema no se acerca tanto a la inestabilidad.  
2. **Robustez** – mejor tolerancia a ruidos y no linealidades cuando se ajusta la histéresis.

---

## 3. Impacto del Ciclo de Trabajo del SSR

El horno usa un **SSR (Solid-State Relay)** controlado por modulación *duty-cycle* (0–100 %).  
El autotuning opera en **modo todo/nada** \((100 \% / 0 \%)\) para que el relé actúe como excitador de relé puro.  

Al pasar a control PID, el *duty* resultante se escala linealmente (0–100 %) y se modula durante la ventana de tiempo fija configurada en `pid_config.sample_time_ms`.

---

## 4. Bibliografía

1. J. G. Ziegler & N. B. Nichols, *Optimum settings for automatic controllers*, **Trans. ASME**, 1942.  
2. K. J. Åström & T. Hägglund, *Automatic tuning of simple regulators with specifications on phase and amplitude margins*, **Automatica**, 1984.  
3. K. J. Åström & T. Hägglund, *PID Controllers: Theory, Design, and Tuning*, Instrument Society of America, 1995. 