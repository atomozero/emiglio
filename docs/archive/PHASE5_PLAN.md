# Fase 5: Interfaccia Utente (UI) - Piano di Implementazione

## Obiettivo
Creare un'interfaccia utente nativa per Haiku OS usando BeAPI/Haiku Interface Kit per:
- Visualizzare dati di mercato in tempo reale
- Gestire strategie (recipes)
- Eseguire e visualizzare backtests
- Monitorare trades e portfolio
- Configurare impostazioni

## Architettura UI

### Stack Tecnologico
- **Framework**: Haiku Interface Kit (BeAPI)
- **Layout Manager**: BLayoutBuilder
- **Threading**: BLooper/BHandler per operazioni async
- **Grafica**: BView per chart e grafici
- **Persistenza**: BMessage per settings

### Componenti Principali

```
MainWindow (BWindow)
├── MenuBar
│   ├── File (New Strategy, Open, Save, Quit)
│   ├── View (Dashboard, Backtest, Trades, Settings)
│   └── Help (About, Documentation)
│
├── TabView (BTabView)
│   ├── DashboardView
│   │   ├── Market Overview
│   │   ├── Active Strategies
│   │   └── Recent Trades
│   │
│   ├── BacktestView
│   │   ├── Strategy Selector
│   │   ├── Date Range Picker
│   │   ├── Run Button
│   │   ├── Results Panel
│   │   │   ├── Metrics (Sharpe, Drawdown, etc.)
│   │   │   ├── Equity Curve Chart
│   │   │   └── Trade List
│   │   └── Export Button (PDF/CSV)
│   │
│   ├── RecipeEditorView
│   │   ├── Recipe Metadata
│   │   ├── Indicators Config
│   │   ├── Entry Conditions Builder
│   │   ├── Exit Conditions Builder
│   │   ├── Risk Management
│   │   └── Save/Load Buttons
│   │
│   ├── TradesView
│   │   ├── Open Positions Table
│   │   ├── Closed Trades Table
│   │   └── Trade Details Panel
│   │
│   ├── ChartView
│   │   ├── Candlestick Chart
│   │   ├── Indicators Overlay
│   │   ├── Volume Bars
│   │   └── Timeframe Selector
│   │
│   └── SettingsView
│       ├── Exchange API Keys
│       ├── Database Path
│       ├── Logging Level
│       └── UI Preferences
│
└── StatusBar (BStatusBar)
    ├── Connection Status
    ├── Last Update Time
    └── CPU/Memory Usage
```

## Piano di Implementazione

### Milestone 1: Finestra Principale e Navigazione (Priorità: ALTA)
**Obiettivo**: Creare la struttura base dell'applicazione

**File da creare**:
1. **MainWindow.h/.cpp**
   - Extends BWindow
   - Setup menu bar
   - Create tab view
   - Handle window events
   - Implement keyboard shortcuts

2. **Application.cpp** (già parzialmente in core/)
   - BApplication subclass
   - Launch MainWindow
   - Handle app lifecycle

**Funzionalità**:
- Finestra ridimensionabile (min 1024x768)
- Menu bar con File/View/Help
- Tab view per navigare tra viste
- Gestione chiusura applicazione
- About dialog

**Test**: Aprire applicazione, navigare tra tab, chiudere

---

### Milestone 2: BacktestView - Integrazione Fase 4 (Priorità: ALTA)
**Obiettivo**: Permettere esecuzione backtests da UI

**File da creare**:
1. **BacktestView.h/.cpp**
   - Recipe selector (BMenuField)
   - Date range pickers (BDateControl)
   - Run button (BButton)
   - Progress bar (BStatusBar)
   - Results table (BColumnListView)
   - Metrics panel (BGroupView)

**Integrazione**:
```cpp
// Pseudo-code
void BacktestView::RunBacktest() {
    // Load recipe
    Recipe recipe;
    RecipeLoader loader;
    loader.loadFromFile(selectedRecipe, recipe);

    // Load candles from DB
    DataStorage storage;
    auto candles = storage.getCandles(exchange, symbol, timeframe, startDate, endDate);

    // Run backtest in thread
    BacktestSimulator simulator(recipe, config);
    BacktestResult result = simulator.run(candles);

    // Analyze
    PerformanceAnalyzer analyzer;
    analyzer.analyze(result);

    // Update UI
    DisplayResults(result);
}
```

**Funzionalità**:
- Seleziona recipe da lista
- Scegli date range
- Configura parametri (capital, commission, slippage)
- Esegui backtest (con progress)
- Mostra risultati:
  - Metriche chiave (Total Return, Sharpe, Max DD, Win Rate)
  - Lista trades (con filtri)
  - Equity curve (placeholder per ora)
- Esporta report (text/JSON)

**Test**: Eseguire backtest con recipe esistente, verificare risultati

---

### Milestone 3: RecipeEditorView - Editor Strategie (Priorità: MEDIA)
**Obiettivo**: Creare e modificare strategie visualmente

**File da creare**:
1. **RecipeEditorView.h/.cpp**
   - Name/description fields (BTextControl)
   - Market config (exchange, symbol, timeframe)
   - Indicators list (BListView + Add/Remove)
   - Conditions builder (BBox)
     - Rule list (BColumnListView)
     - Add rule button
     - Logic selector (AND/OR)
   - Risk management fields
   - Save/Load buttons

**Funzionalità**:
- Crea nuova strategy
- Carica strategy esistente
- Modifica metadata
- Aggiungi/rimuovi indicatori
- Costruisci entry/exit conditions visualmente
- Imposta stop-loss/take-profit
- Valida recipe
- Salva su file JSON

**Test**: Creare recipe da UI, salvare, ricaricare, verificare correttezza JSON

---

### Milestone 4: DashboardView - Overview (Priorità: MEDIA)
**Obiettivo**: Dashboard con panoramica sistema

**File da creare**:
1. **DashboardView.h/.cpp**
   - Active strategies panel
   - Recent trades list
   - Portfolio summary (cash, equity, P&L)
   - System status (DB, API connections)

**Funzionalità**:
- Mostra strategie attive
- Ultime 10 trades
- Statistiche portfolio
- Quick actions (run backtest, view trades)

**Test**: Aprire dashboard, verificare dati visualizzati

---

### Milestone 5: TradesView - Storico Trade (Priorità: MEDIA)
**Obiettivo**: Visualizzare e analizzare trades

**File da creare**:
1. **TradesView.h/.cpp**
   - Trades table (BColumnListView)
   - Filters (date range, strategy, P&L)
   - Trade details panel
   - Export button

**Funzionalità**:
- Lista tutti i trades (open + closed)
- Filtra per data/strategy/tipo
- Mostra dettagli trade selezionato
- Esporta CSV

**Test**: Visualizzare trades da backtest precedente

---

### Milestone 6: ChartView - Grafici (Priorità: BASSA)
**Obiettivo**: Visualizzare candlestick chart con indicatori

**File da creare**:
1. **ChartView.h/.cpp**
   - Custom BView per rendering
   - Candlestick drawing
   - Indicators overlay
   - Zoom/pan controls

**Funzionalità**:
- Disegna candlestick chart
- Overlay indicatori (SMA, EMA, RSI, etc.)
- Barre volume
- Zoom e pan
- Crosshair con valori

**Note**: Questa è la milestone più complessa graficamente. Può essere implementata per ultima o usare libreria esterna (BChart se disponibile).

**Test**: Visualizzare chart con dati storici

---

### Milestone 7: SettingsView - Configurazione (Priorità: BASSA)
**Obiettivo**: Configurare applicazione

**File da creare**:
1. **SettingsView.h/.cpp**
   - Settings form
   - Apply/Reset buttons
   - Settings persistence

**Funzionalità**:
- Database path
- Log level
- API keys (masked input)
- UI theme preferences
- Default values

**Test**: Modificare settings, riavviare app, verificare persistenza

---

## Dettagli Implementazione

### Threading e Performance

Per operazioni lunghe (backtest, caricamento dati):
```cpp
class BacktestThread : public BLooper {
public:
    void Run() {
        // Execute backtest
        BacktestResult result = simulator.run(candles);

        // Send result back to UI via BMessage
        BMessage msg(MSG_BACKTEST_COMPLETE);
        msg.AddPointer("result", &result);
        targetView->PostMessage(&msg);
    }
};
```

### Gestione Errori

Tutti i metodi UI devono gestire errori gracefully:
```cpp
try {
    result = simulator.run(candles);
} catch (const std::exception& e) {
    BAlert* alert = new BAlert("Error",
        e.what(),
        "OK", nullptr, nullptr,
        B_WIDTH_AS_USUAL, B_STOP_ALERT);
    alert->Go();
}
```

### Persistenza Settings

Usare BMessage per salvare/caricare settings:
```cpp
BMessage settings;
settings.AddString("db_path", "/boot/home/emilio.db");
settings.AddInt32("log_level", LOG_INFO);

BFile file("/boot/home/config/settings/Emiglio", B_WRITE_ONLY | B_CREATE_FILE);
settings.Flatten(&file);
```

## Stile e Design

### Colori
- **Positivo**: Verde (#4CAF50)
- **Negativo**: Rosso (#F44336)
- **Neutro**: Grigio (#9E9E9E)
- **Background**: Bianco (#FFFFFF)
- **Testo**: Nero (#212121)

### Font
- **Titoli**: be_bold_font (14pt)
- **Normale**: be_plain_font (12pt)
- **Monospace**: be_fixed_font (per numeri, codice)

### Layout
- Padding standard: 10px
- Spacing tra elementi: 5px
- Bordi arrotondati: 5px
- Minimo dimensione bottoni: 80x30px

## Priorità di Implementazione

### Sprint 1 (Fondamenta - 1 settimana)
1. MainWindow con tab view ✓
2. BacktestView base (senza chart) ✓
3. Integrazione BacktestSimulator ✓

### Sprint 2 (Editor - 1 settimana)
4. RecipeEditorView ✓
5. Validazione e save/load recipes ✓

### Sprint 3 (Monitoring - 3 giorni)
6. DashboardView ✓
7. TradesView ✓

### Sprint 4 (Polish - opzionale)
8. ChartView (se tempo)
9. SettingsView ✓

## Testing

### Unit Test
Limitati per UI, focus su:
- Validazione input
- Conversione dati
- Logica business in view models

### Integration Test
- Backtest end-to-end da UI
- Save/load recipes
- Export reports

### Manual Test
- Checklist UI per ogni view
- Test responsiveness (resize window)
- Keyboard navigation
- Error handling

## Deliverable

Al completamento Fase 5:
- [ ] Applicazione Haiku nativa funzionante
- [ ] Esecuzione backtests da UI
- [ ] Editor strategie visuale
- [ ] Visualizzazione risultati e trades
- [ ] Documentazione UI (user guide)
- [ ] Screenshot/demo video

## Risorse

- **Haiku Interface Kit**: https://www.haiku-os.org/docs/api/interface.html
- **BeBook**: https://www.haiku-os.org/legacy-docs/bebook/
- **Layout Management**: https://www.haiku-os.org/docs/api/layout/
- **Best Practices**: Seguire HIG (Human Interface Guidelines) di Haiku

## Note Tecniche

### Build System
- Usare makefile-engine di Haiku
- Linking con libbe.so
- Resource file (.rdef) per icone/metadata

### Deployment
- Creare pacchetto .hpkg per distribuzione
- Includere dipendenze (SQLite, OpenSSL)
- Installer con default settings

---

**Stato**: 📋 Piano Pronto
**Prossimo Step**: Implementare Milestone 1 (MainWindow)
