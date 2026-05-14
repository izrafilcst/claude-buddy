# Wokwi Demo Mode — Design Spec

**Data:** 2026-05-13  
**Projeto:** Claude Tamagotchi Monitor  
**Escopo:** Pasta `demo/` com mock agent + integração Wokwi VS Code para simular o firmware sem hardware físico

---

## Visão Geral

O modo demo permite rodar o firmware C++ compilado do Claude Tamagotchi Monitor dentro do simulador Wokwi (extensão VS Code), com um servidor mock substituindo o agente Python real. O firmware não precisa de nenhuma modificação — o mock serve o mesmo JSON na mesma porta.

O mock agent suporta dois modos de operação:
- **Auto-ciclo:** avança automaticamente pelos cenários a cada N segundos (padrão: 30s)
- **Controle manual:** endpoint HTTP para forçar qualquer cenário sem reiniciar o servidor

---

## Arquitetura

```
VS Code
├── Extensão Wokwi
│   ├── lê demo/wokwi.toml → carrega firmware/.pio/build/esp32s3/firmware.elf
│   └── simula ESP32-S3 + ILI9341 240×320 + piezo (virtual)
│       └── WiFi virtual → localhost:8266
│
└── Terminal: python demo/mock_agent.py
    ├── GET /status          → JSON do cenário atual
    ├── GET /control?scenario=<nome>  → troca cenário imediatamente
    └── GET /scenarios       → lista cenários disponíveis
```

O firmware detecta o agente mock exatamente como detecta o agente real — mesmo host (`127.0.0.1`), mesma porta (`8266`), mesmo path (`/status`), mesmo formato JSON.

---

## Estrutura de Arquivos

```
demo/
├── wokwi.toml        # configuração do simulador Wokwi
├── diagram.json      # circuito virtual: ESP32-S3 + ILI9341 + piezo
├── mock_agent.py     # servidor HTTP mock (Python stdlib apenas)
└── scenarios.json    # payloads JSON para cada estado do mascote
```

---

## `wokwi.toml`

```toml
[wokwi]
version = 1
firmware = '../firmware/.pio/build/esp32s3/firmware.elf'
elf = '../firmware/.pio/build/esp32s3/firmware.elf'

[wifi]
ssid = "Wokwi-GUEST"
password = ""
```

O path `../firmware/.pio/...` é relativo à pasta `demo/`. O usuário deve compilar o firmware com `pio run` antes de iniciar o simulador.

A seção `[wifi]` pré-popula o NVS do Wokwi com as credenciais, fazendo o `WiFiManager.autoConnect()` conectar automaticamente sem abrir o portal captivo. Sem ela, o firmware ficaria preso aguardando configuração de WiFi na primeira boot.

---

## `diagram.json`

Circuito virtual com os pinos mapeados conforme `platformio.ini`:

| Pino ESP32-S3 | Função | Periférico |
|---|---|---|
| GPIO11 | MOSI | ILI9341 |
| GPIO18 | SCLK | ILI9341 |
| GPIO13 | MISO | ILI9341 |
| GPIO10 | CS | ILI9341 |
| GPIO14 | DC | ILI9341 |
| GPIO9 | RST | ILI9341 |
| GPIO12 | TOUCH_CS | ILI9341 (touch) |
| GPIO48 | BL (backlight) | ILI9341 |
| GPIO47 | Piezo | Buzzer |

Componentes Wokwi utilizados: `board-esp32-s3-devkitc-1`, `wokwi-ili9341`, `wokwi-buzzer`.

---

## `scenarios.json`

Formato dos campos segue exatamente o que `api_client.cpp` parseia:

```json
{
  "radiant":   { "quota_percent": 10, "sessions_active": 1, "sessions_details": [{"pid": 1001, "uptime_min": 45}],  "reset_minutes": 320, "plan": "pro" },
  "content":   { "quota_percent": 40, "sessions_active": 2, "sessions_details": [{"pid": 1001, "uptime_min": 120}, {"pid": 1002, "uptime_min": 20}], "reset_minutes": 200, "plan": "pro" },
  "normal":    { "quota_percent": 58, "sessions_active": 1, "sessions_details": [{"pid": 1003, "uptime_min": 90}],  "reset_minutes": 130, "plan": "pro" },
  "tired":     { "quota_percent": 72, "sessions_active": 1, "sessions_details": [{"pid": 1004, "uptime_min": 240}], "reset_minutes": 80,  "plan": "pro" },
  "exhausted": { "quota_percent": 88, "sessions_active": 3, "sessions_details": [{"pid": 1005, "uptime_min": 480}, {"pid": 1006, "uptime_min": 60}, {"pid": 1007, "uptime_min": 15}], "reset_minutes": 20, "plan": "pro" },
  "offline":   { "quota_percent": -1, "sessions_active": 0, "sessions_details": [], "reset_minutes": -1, "plan": "unknown" }
}
```

Cenário `error` não está em `scenarios.json` — é tratado diretamente no mock agent: retorna HTTP 500, o firmware não consegue parsear e entra em estado CONFUSED/SEARCHING.

**Mapeamento estado → cenário:**

| Cenário | `quota_percent` | `sessions_active` | Estado do mascote |
|---|---|---|---|
| `radiant` | 10% | 1 | RADIANT |
| `content` | 40% | 2 | CONTENT |
| `normal` | 58% | 1 | NORMAL |
| `tired` | 72% | 1 | TIRED |
| `exhausted` | 88% | 3 | EXHAUSTED |
| `offline` | -1 | 0 | OFF → SEARCHING |
| `error` | — | — | CONFUSED (HTTP 500) |

---

## `mock_agent.py`

### Inicialização

```bash
# Auto-ciclo (padrão: 30s por cenário)
python demo/mock_agent.py

# Auto-ciclo com intervalo customizado
python demo/mock_agent.py --interval 10

# Estado fixo (desativa auto-ciclo)
python demo/mock_agent.py --scenario tired
```

### Endpoints

**`GET /status`**  
Retorna o payload JSON do cenário atual. Se o cenário ativo for `error`, retorna HTTP 500 com body vazio.

**`GET /control?scenario=<nome>`**  
Troca o cenário ativo imediatamente. Reinicia o timer do auto-ciclo. Retorna `{"ok": true, "scenario": "<nome>"}`. Se o nome for inválido, retorna HTTP 400.

**`GET /scenarios`**  
Retorna lista de cenários disponíveis: `{"scenarios": ["radiant", "content", ..., "error"]}`.

### Auto-ciclo

Thread daemon separada. Sequência fixa:
```
radiant → content → normal → tired → exhausted → offline → error → radiant → ...
```

Quando o usuário usa `/control`, o timer reseta (o próximo avanço automático ocorre N segundos após a troca manual).

### Implementação

- Python stdlib apenas (`http.server`, `threading`, `json`, `argparse`)
- Carrega `scenarios.json` no boot (path relativo ao script)
- Thread de ciclo usa `threading.Event` para sleep interrompível (permite shutdown limpo com Ctrl+C)
- Log no stdout: `[mock] scenario=tired (auto)` / `[mock] scenario=radiant (manual)`

---

## Fluxo de Uso

1. Compilar firmware: `cd firmware && pio run`
2. Iniciar mock agent: `python demo/mock_agent.py`
3. Abrir VS Code na raiz do projeto
4. `F1 → Wokwi: Start Simulator` (extensão detecta `demo/wokwi.toml`)
5. Display virtual aparece na aba Wokwi — WiFi conecta automaticamente ao mock
6. Para forçar um estado: `curl "localhost:8266/control?scenario=exhausted"`

---

## O Que Não Está No Escopo

- Modificações no firmware (o firmware existente funciona sem alterações)
- Suporte ao wokwi.com (WiFi não alcança localhost nesse modo)
- Mock agent para planos diferentes de "pro" (o firmware não diferencia por plano)
- Testes automatizados para o mock agent (é uma ferramenta de dev, não produção)
