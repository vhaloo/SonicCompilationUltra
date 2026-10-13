#include "localization.h"
#include <SDL.h>
#include <cstring>
namespace scu {
struct Text {
    const char *key;
    const char *text[4];
};
#define T(key, en, fr, es, ja)                                                                     \
    {                                                                                              \
        key, {                                                                                     \
            en, fr, es, ja                                                                         \
        }                                                                                          \
    }
static const Text texts[] = {
    T("enemypreview", "Preview distant enemy spawns", "Aperçu des apparitions d’ennemis éloignés",
      "Vista previa de enemigos lejanos", "遠くの敵の出現位置を表示"),
    T("enemystyle", "Enemy preview style", "Style de l’aperçu des ennemis",
      "Estilo de vista previa de enemigos", "敵のプレビュー表示"),
    T("enemyopacity", "Enemy preview opacity", "Opacité de l’aperçu des ennemis",
      "Opacidad de los enemigos", "敵のプレビュー不透明度"),
    T("enemyspritesmarkers", "Ghost sprites + spawn markers", "Sprites transparents et repères",
      "Sprites transparentes y marcadores", "半透明の敵と出現マーカー"),
    T("enemysprites", "Ghost sprites", "Sprites transparents", "Sprites transparentes",
      "半透明の敵"),
    T("enemymarkers", "Spawn markers", "Repères d’apparition", "Marcadores de aparición",
      "出現マーカー"),
    T("enemyhelp",
      "Expanded views show translucent enemies at their authored spawn points until the native "
      "sprite is visible. Defeated enemies are hidden when the game remembers them. Markers stay "
      "readable when zoomed out and cover unavailable sprite art. These are spawn previews, not "
      "moving enemies or attack predictions. Sonic 1, 2, 3 and Knuckles platform stages only.",
      "Les vues étendues montrent les ennemis transparents à leur point d’apparition jusqu’à ce "
      "que leur sprite natif soit visible. Les ennemis détruits sont masqués lorsque le jeu s’en "
      "souvient. Les repères restent lisibles en dézoom et remplacent les images indisponibles. "
      "Ils indiquent les apparitions, pas les mouvements ni les attaques. Niveaux de plateforme "
      "de Sonic 1, 2, 3 et Knuckles uniquement.",
      "Las vistas ampliadas muestran enemigos transparentes en sus puntos de aparición hasta "
      "que el sprite original es visible. Los derrotados se ocultan si el juego los recuerda. "
      "Los marcadores siguen legibles al alejar la vista y cubren imágenes no disponibles. Son "
      "puntos de aparición, no movimientos ni predicciones de ataques. Solo niveles de "
      "plataformas de Sonic 1, 2, 3 y Knuckles.",
      "拡張表示では、元の敵が見えるまで出現位置に半透明の敵を表示します。ゲームが記録した倒した敵は"
      "非表示になります。縮小時や画像が使えない場合はマーカーで位置を示します。敵の移動や攻撃を予測"
      "する表示ではありません。Sonic 1、2、3 と Knuckles の通常ステージで使えます。"),
    T("overviewstyle", "Overview terrain", "Décor du niveau entier", "Terreno en vista completa",
      "全体表示の地形"),
    T("nativeart", "Original game artwork", "Graphismes d’origine", "Gráficos originales",
      "元のゲーム画像"),
    T("terrainmap", "Clean terrain map", "Carte de terrain épurée", "Mapa de terreno limpio",
      "地形マップ"),
    T("maphelp",
      "Automatic uses a clean schematic map for Sonic 3-era streamed terrain. Original artwork "
      "remains available; the live player view always uses the native image.",
      "Automatique utilise une carte schématique pour les décors chargés progressivement de Sonic "
      "3. Les graphismes d’origine restent disponibles; la vue du joueur garde l’image native.",
      "Automático usa un mapa esquemático para el terreno que Sonic 3 carga progresivamente. Los "
      "gráficos originales siguen disponibles; la vista del jugador conserva la imagen nativa.",
      "自動では Sonic 3 "
      "系の順次読み込まれる地形にマップ表示を使います。元の画像も選べます。プレイヤー映像は常に元の"
      "画面です。"),
    T("importshort", "ROM…", "ROM…", "ROM…", "ROM…"),
    T("upshort", "Up", "Parent", "Arriba", "上へ"),
    T("pathshort", "Path", "Chemin", "Ruta", "パス"),
    T("playerinset", "Live player view in overview", "Vue du joueur dans le niveau entier",
      "Vista del jugador en el nivel completo", "全体表示にプレイヤー映像を追加"),
    T("overviewcamera",
      "Whole-stage view stays fixed. Focus on player or choose a camera mode to move closer.",
      "Le niveau entier reste fixe. Centre sur le joueur ou choisis une caméra pour te rapprocher.",
      "La vista completa permanece fija. Centra en el jugador o elige una cámara para acercarte.",
      "全体表示は固定です。プレイヤーへ移動するか、カメラを選ぶと近づけます。"),
    T("menushort", "Menu: Esc / Select", "Menu : Échap / Select", "Menú: Esc / Select",
      "メニュー: Esc / Select"),
    T("navxbox", "A: confirm · B: back · LB/RB: tabs · LT/RT: scroll · Start: resume",
      "A : valider · B : retour · LB/RB : onglets · LT/RT : défiler · Start : reprendre",
      "A: aceptar · B: volver · LB/RB: pestañas · LT/RT: desplazar · Start: continuar",
      "A: 決定 · B: 戻る · LB/RB: タブ · LT/RT: スクロール · Start: 再開"),
    T("navps", "Cross: confirm · Circle: back · L1/R1: tabs · L2/R2: scroll · Options: resume",
      "Croix : valider · Rond : retour · L1/R1 : onglets · L2/R2 : défiler · Options : reprendre",
      "Cruz: aceptar · Círculo: volver · L1/R1: pestañas · L2/R2: desplazar · Options: continuar",
      "クロス: 決定 · サークル: 戻る · L1/R1: タブ · L2/R2: スクロール · Options: 再開"),
    T("navnintendo",
      "Bottom button: confirm · Right button: back · L/R: tabs · ZL/ZR: scroll · +: resume",
      "Bouton du bas : valider · Bouton droit : retour · L/R : onglets · ZL/ZR : défiler · + : "
      "reprendre",
      "Botón inferior: aceptar · Botón derecho: volver · L/R: pestañas · ZL/ZR: desplazar · +: "
      "continuar",
      "下ボタン: 決定 · 右ボタン: 戻る · L/R: タブ · ZL/ZR: スクロール · +: 再開"),
    T("autosettings",
      "D-pad / left stick: navigate · Confirm, then left/right: adjust · Keyboard: arrows, Enter, "
      "Esc; Ctrl+Tab switches tabs. Settings save automatically.",
      "Croix / stick gauche : naviguer · Valider, puis gauche/droite : régler · Clavier : flèches, "
      "Entrée, Échap; Ctrl+Tab change d’onglet. Sauvegarde automatique.",
      "Cruceta / stick izquierdo: navegar · Aceptar y luego izquierda/derecha: ajustar · Teclado: "
      "flechas, Enter, Esc; Ctrl+Tab cambia de pestaña. Ajustes guardados automáticamente.",
      "十字キー・左スティック: 移動 · 決定の後に左右: 調整 · キーボード: "
      "矢印、Enter、Esc。Ctrl+Tab でタブ切替。設定は自動保存。"),
    T("prevsection", "<", "<", "<", "<"),
    T("nextsection", ">", ">", ">", ">"),
    T("menulayout", "Options menu layout", "Disposition du menu", "Diseño del menú",
      "設定メニューの配置"),
    T("fullmenu", "Fullscreen menu", "Menu plein écran", "Menú a pantalla completa",
      "全画面メニュー"),
    T("compactmenu", "Compact overlay", "Fenêtre compacte", "Ventana compacta", "コンパクト表示"),
    T("smalltext", "Small", "Petit", "Pequeño", "小"),
    T("largetext", "Large", "Grand", "Grande", "大"),
    T("hugetext", "Extra large", "Très grand", "Muy grande", "特大"),
    T("contrast", "High-contrast interface", "Interface à contraste élevé",
      "Interfaz de alto contraste", "高コントラスト UI"),
    T("display", "Fullscreen display", "Écran pour le plein écran", "Pantalla para modo completo",
      "全画面に使うディスプレイ"),
    T("currentdisplay", "Current display", "Écran actuel", "Pantalla actual", "現在のディスプレイ"),
    T("overviewhelp",
      "Default: the whole current stage fits on screen, with a player marker. Puzzle, 3D, "
      "special-stage and title screens fit their native image. Expanded stage art depends on the "
      "game’s loaded tiles.",
      "Par défaut : le niveau actuel tient à l’écran, avec un repère du joueur. Puzzles, 3D, "
      "étapes spéciales et titres gardent leur image native. Le décor étendu dépend des tuiles "
      "chargées par le jeu.",
      "Por defecto: el nivel actual cabe en pantalla, con un marcador del jugador. Puzles, 3D, "
      "fases especiales y títulos ajustan su imagen original. Los gráficos ampliados dependen de "
      "las piezas cargadas por el juego.",
      "既定では現在のステージ全体とプレイヤーマーカーを表示します。パズル、3D、特殊ステージ、タイト"
      "ル画面は元の映像を全体表示します。拡張描画は読み込まれたタイルに依存します。"),
    T("showstage", "Show whole stage", "Voir le niveau entier", "Ver todo el nivel",
      "ステージ全体を表示"),
    T("focusplayer", "Focus on player", "Centrer sur le joueur", "Centrar en el jugador",
      "プレイヤーを中央に"),
    T("dismiss", "Dismiss notice", "Fermer le message", "Cerrar aviso", "通知を閉じる"),
    T("readyonly", "Show imported games only", "Afficher seulement les jeux importés",
      "Mostrar solo juegos importados", "読み込み済みのゲームのみ表示"),
    T("continue", "Continue last game", "Continuer le dernier jeu", "Continuar último juego",
      "前回のゲームを続ける"),
    T("padlabels", "Controller button hints", "Repères des boutons de manette",
      "Indicaciones del mando", "ゲームパッドのボタン表示"),
    T("jumpbuttons", "Gameplay face-button layout", "Boutons d’action en jeu",
      "Botones de acción en juego", "ゲーム中のボタン配置"),
    T("jumpclassic", "Genesis A = X, B = A, C = B", "Genesis A = X, B = A, C = B",
      "Genesis A = X, B = A, C = B", "Genesis A = X、B = A、C = B"),
    T("jumpabc", "Genesis A = A, B = B, C = X", "Genesis A = A, B = B, C = X",
      "Genesis A = A, B = B, C = X", "Genesis A = A、B = B、C = X"),
    T("jumpbac", "Genesis A = B, B = A, C = X", "Genesis A = B, B = A, C = X",
      "Genesis A = B, B = A, C = X", "Genesis A = B、B = A、C = X"),
    T("quickcontrols", "Enable in-game controller shortcuts", "Raccourcis manette en jeu",
      "Atajos del mando en juego", "ゲーム中のショートカットを有効化"),
    T("quickhelp",
      "During gameplay, hold the right shoulder: Up/Down = zoom; top face button = overview; left "
      "face button = freeze/follow; right face button = focus player; Start = screenshot. Release "
      "the shoulder to play. Select/View always opens options. LB rewinds when enabled.",
      "En jeu, maintiens le bouton d’épaule droit : haut/bas = zoom; bouton du haut = niveau "
      "entier; bouton gauche = figer/suivre; bouton droit = centrer; Start = capture. Relâche "
      "l’épaule pour jouer. Select/View ouvre les options. LB rembobine si activé.",
      "En juego, mantén el hombro derecho: arriba/abajo = zoom; botón superior = nivel entero; "
      "izquierdo = fijar/seguir; derecho = centrar; Start = captura. Suelta el hombro para jugar. "
      "Select/View abre opciones. LB rebobina si está activado.",
      "ゲーム中は右ショルダーを押しながら操作: "
      "上下でズーム、上ボタンで全体表示、左ボタンで固定・追従、右ボタンでプレイヤーへ、Start "
      "で撮影。ショルダーを離すと通常操作。Select/View で設定。巻き戻しを有効にすると LB "
      "で戻せます。"),
    T("nopad",
      "No controller connected. Connect one at any time; keyboard controls remain available.",
      "Aucune manette connectée. Branche-la à tout moment; le clavier reste disponible.",
      "No hay mando conectado. Puedes conectarlo en cualquier momento; el teclado sigue "
      "disponible.",
      "ゲームパッドが接続されていません。いつでも接続でき、キーボードも使用できます。"),
    T("confirmactions", "Confirm resets, loads and overwrites",
      "Confirmer les remises à zéro et remplacements",
      "Confirmar reinicios, cargas y sobrescrituras", "リセット・読み込み・上書きを確認"),
    T("confirmquit", "Confirm before quitting", "Confirmer avant de quitter",
      "Confirmar antes de salir", "終了前に確認"),
    T("resetconfirm",
      "Restart this game? Current unsaved progress will be lost. Saved slots are kept.",
      "Redémarrer ce jeu ? La progression non sauvegardée sera perdue. Les sauvegardes sont "
      "conservées.",
      "¿Reiniciar este juego? Se perderá el progreso no guardado. Las partidas guardadas se "
      "conservan.",
      "ゲームを再起動しますか？未保存の進行状況が失われます。セーブスロットは保持されます。"),
    T("defaultsconfirm",
      "Restore default options? Cartridge paths, saves, language and mute choice are kept.",
      "Rétablir les options par défaut ? Cartouches, sauvegardes, langue et choix de sourdine sont "
      "conservés.",
      "¿Restaurar opciones predeterminadas? Se conservan cartuchos, partidas, idioma y silencio.",
      "設定を既定に戻しますか？カートリッジの場所、セーブ、言語、ミュートは保持します。"),
    T("loadconfirm", "Load this slot? Current progress will be replaced by the saved checkpoint.",
      "Charger ce créneau ? La progression sera remplacée par la sauvegarde.",
      "¿Cargar esta ranura? El progreso actual se reemplazará por la partida guardada.",
      "スロットを読み込みますか？現在の進行状況は保存時点に置き換わります。"),
    T("overwriteconfirm", "Overwrite this save slot with your current progress?",
      "Remplacer cette sauvegarde par la progression actuelle ?",
      "¿Sobrescribir esta ranura con tu progreso actual?",
      "このスロットを現在の進行状況で上書きしますか？"),
    T("switchconfirm",
      "Switch games? Automatic resume preserves your current progress when enabled.",
      "Changer de jeu ? La reprise automatique conserve la progression si elle est activée.",
      "¿Cambiar de juego? La reanudación automática conserva el progreso si está activada.",
      "ゲームを切り替えますか？自動再開が有効な場合、進行状況を保存します。"),
    T("defaultsdone", "Default options restored.", "Options par défaut rétablies.",
      "Opciones predeterminadas restauradas.", "既定の設定に戻しました。"),
    T("resetdone", "Game restarted.", "Jeu redémarré.", "Juego reiniciado.",
      "ゲームを再起動しました。"),
    T("saved", "Progress saved.", "Progression sauvegardée.", "Progreso guardado.",
      "進行状況を保存しました。"),
    T("loaded", "Checkpoint loaded.", "Sauvegarde chargée.", "Partida cargada.",
      "セーブを読み込みました。"),
    T("slothelp", "Choose a slot, then Save or Load. Empty slots cannot be loaded.",
      "Choisis un créneau, puis Sauvegarder ou Charger. Un créneau vide ne peut pas être chargé.",
      "Elige una ranura y después Guardar o Cargar. No se pueden cargar ranuras vacías.",
      "スロットを選んで保存・読み込みを行います。空のスロットは読み込めません。"),
    T("occupied", "Saved", "Sauvegardé", "Guardado", "保存済み"),
    T("emptyslot", "Empty", "Vide", "Vacío", "空"),
    T("slotnumber", "Slot", "Créneau", "Ranura", "スロット"),
    T("editpath", "Edit path", "Modifier le chemin", "Editar ruta", "パスを編集"),
    T("applypath", "Go", "Ouvrir", "Ir", "開く"),
    T("backspace", "Delete", "Effacer", "Borrar", "削除"),
    T("locations", "Drives / locations", "Disques / emplacements", "Unidades / ubicaciones",
      "ドライブ・場所"),
    T("pathmissing", "This folder is missing or cannot be read. Choose another location.",
      "Ce dossier est absent ou illisible. Choisis un autre emplacement.",
      "La carpeta no existe o no se puede leer. Elige otra ubicación.",
      "フォルダーがないか、読み込めません。別の場所を選んでください。"),
    T("nofiles", "No matching files here. Use Up or Drives / locations to keep browsing.",
      "Aucun fichier compatible ici. Utilise Parent ou Disques / emplacements.",
      "No hay archivos compatibles aquí. Usa Arriba o Unidades / ubicaciones.",
      "対応するファイルがありません。上のフォルダーやドライブから探してください。"),
    T("gamename", "Game", "Jeu", "Juego", "ゲーム"),
    T("bytes", "Bytes", "Octets", "Bytes", "バイト"),
    T("library", "Library", "Collection", "Colección", "ライブラリ"),
    T("video", "Screen", "Écran", "Pantalla", "画面"),
    T("camera", "Camera & HUD", "Caméra et interface", "Cámara e interfaz", "カメラと表示"),
    T("atmosphere", "Background", "Décor", "Fondo", "背景"),
    T("sound", "Sound", "Son", "Sonido", "音声"),
    T("controls", "Controls", "Commandes", "Controles", "操作"),
    T("gameplay", "Gameplay", "Jeu", "Juego", "ゲーム"),
    T("saves", "Saves", "Sauvegardes", "Partidas", "セーブ"),
    T("about", "Help & credits", "Aide et crédits", "Ayuda y créditos", "ヘルプとクレジット"),
    T("tagline", "Your cartridges. A much bigger view.",
      "Tes cartouches. Une vue beaucoup plus grande.", "Tus cartuchos. Una vista mucho más amplia.",
      "自分のカートリッジで、もっと広い世界へ。"),
    T("menuhint", "Menu: Esc / F10  ·  Gamepad Select / View",
      "Menu : Échap / F10  ·  Select / View sur la manette",
      "Menú: Esc / F10  ·  Select / View en el mando",
      "メニュー: Esc / F10 ・ ゲームパッド Select / View"),
    T("resume", "Resume", "Reprendre", "Continuar", "再開"),
    T("play", "Play", "Jouer", "Jugar", "プレイ"),
    T("import", "Choose ROM…", "Choisir une ROM…", "Elegir ROM…", "ROM を選ぶ…"),
    T("folder", "Import folder…", "Importer un dossier…", "Importar carpeta…",
      "フォルダーを読み込む…"),
    T("ready", "Ready", "Prêt", "Listo", "準備完了"),
    T("missing", "Choose your cartridge to unlock", "Choisis ta cartouche pour jouer",
      "Elige tu cartucho para jugar", "カートリッジを選ぶとプレイできます"),
    T("derived", "Lock-on built from your cartridges", "Lock-on créé avec tes cartouches",
      "Lock-on creado con tus cartuchos", "自分のカートリッジからロックオンを構成"),
    T("empty", "Start here: choose a ROM or drop it into this window.",
      "Commence ici : choisis une ROM ou dépose-la dans cette fenêtre.",
      "Empieza aquí: elige una ROM o arrástrala a esta ventana.",
      "まず ROM を選ぶか、このウィンドウにドロップしてください。"),
    T("legalshort", "No games are included. Supply a copy you are entitled to use.",
      "Aucun jeu n’est fourni. Utilise une copie que tu as le droit d’utiliser.",
      "No se incluyen juegos. Usa una copia que tengas derecho a utilizar.",
      "ゲームデータは付属しません。利用する権利のあるコピーを用意してください。"),
    T("sourcehelp", "ROM guide", "Guide des ROMs", "Guía de ROMs", "ROM ガイド"),
    T("settings", "Options", "Options", "Opciones", "設定"),
    T("quit", "Quit", "Quitter", "Salir", "終了"),
    T("quitconfirm", "Quit? Progress is kept when automatic resume is enabled.",
      "Quitter ? La reprise automatique conserve la progression si elle est activée.",
      "¿Salir? La reanudación automática conserva el progreso si está activada.",
      "終了しますか？自動再開が有効な場合、進行状況を保存します。"),
    T("cancel", "Cancel", "Annuler", "Cancelar", "キャンセル"),
    T("yes", "Yes", "Oui", "Sí", "はい"),
    T("language", "Language", "Langue", "Idioma", "言語"),
    T("auto", "Automatic", "Automatique", "Automático", "自動"),
    T("preset", "Performance preset", "Profil de performances", "Perfil de rendimiento",
      "パフォーマンス設定"),
    T("low", "Lightweight / integrated GPU", "Léger / carte intégrée", "Ligero / gráfica integrada",
      "軽量・内蔵 GPU"),
    T("balanced", "Balanced", "Équilibré", "Equilibrado", "バランス"),
    T("ultra", "Ultra quality", "Qualité Ultra", "Calidad Ultra", "ウルトラ品質"),
    T("view", "View", "Vue", "Vista", "表示モード"),
    T("pixel", "Pixel world / integer zoom", "Monde en pixels / zoom entier",
      "Mundo en píxeles / zoom entero", "ピクセルワールド・整数ズーム"),
    T("panorama", "Panorama / original height", "Panorama / hauteur d’origine",
      "Panorámica / altura original", "パノラマ・元の高さ"),
    T("overview", "Whole-level overview", "Vue du niveau entier", "Vista de todo el nivel",
      "ステージ全体"),
    T("classic", "Original game framing", "Cadre original du jeu", "Encuadre original del juego",
      "元のゲーム画面"),
    T("zoom", "Integer zoom", "Zoom entier", "Zoom entero", "整数ズーム"),
    T("pixelhelp",
      "At 1×, one game pixel is one physical screen pixel. On smaller screens, zoom out with "
      "Panorama or Overview.",
      "À 1×, un pixel du jeu correspond à un pixel physique de l’écran. Sur un petit écran, "
      "utilise Panorama ou Niveau entier pour dézoomer.",
      "A 1×, un píxel del juego equivale a un píxel físico de pantalla. En pantallas pequeñas, usa "
      "Panorámica o Vista completa.",
      "1× はゲームの 1 ピクセルを画面の物理的な 1 "
      "ピクセルで表示します。小さい画面ではパノラマや全体表示を使えます。"),
    T("fullscreen", "Borderless fullscreen", "Plein écran sans bordure",
      "Pantalla completa sin bordes", "ボーダーレス全画面"),
    T("span", "Span all displays", "Étendre à tous les écrans", "Usar todas las pantallas",
      "すべてのディスプレイに表示"),
    T("vsync", "Vertical sync", "Synchronisation verticale", "Sincronización vertical", "垂直同期"),
    T("nearest", "Sharp integer scaling", "Agrandissement entier net", "Escalado entero nítido",
      "鮮明な整数拡大"),
    T("nativescale", "Menus / puzzle screens scale", "Taille des menus et puzzles",
      "Escala de menús y puzles", "メニュー・パズルの拡大率"),
    T("uiscale", "Menu text size", "Taille du texte des menus", "Tamaño del texto del menú",
      "メニューの文字サイズ"),
    T("workers", "Rendering workers (0 = auto)", "Calculs parallèles (0 = auto)",
      "Hilos de renderizado (0 = auto)", "描画スレッド数 (0 = 自動)"),
    T("soft", "Use software renderer (restart)", "Rendu logiciel (redémarrage)",
      "Renderizado por software (reiniciar)", "ソフトウェア描画を使用 (再起動)"),
    T("scanlines", "Scanline strength", "Intensité des lignes vidéo",
      "Intensidad de líneas de barrido", "走査線の強さ"),
    T("performance", "Show frame rate", "Afficher les performances", "Mostrar rendimiento",
      "フレームレートを表示"),
    T("cameramode", "Camera behaviour", "Comportement de la caméra", "Comportamiento de cámara",
      "カメラの動作"),
    T("bounded", "Follow · stay inside level", "Suivi · rester dans le niveau",
      "Seguir · dentro del nivel", "追従・ステージ内に制限"),
    T("centred", "Follow · centre on player", "Suivi · centrer sur le joueur",
      "Seguir · centrar en jugador", "追従・プレイヤーを中央に"),
    T("fixed", "Fixed camera", "Caméra fixe", "Cámara fija", "固定カメラ"),
    T("free", "Free camera (WASD / right stick)", "Caméra libre (WASD / stick droit)",
      "Cámara libre (WASD / stick derecho)", "自由カメラ (WASD / 右スティック)"),
    T("level", "Centre the whole level", "Centrer le niveau entier", "Centrar todo el nivel",
      "ステージ全体を中央に"),
    T("freeze", "Freeze here", "Figer ici", "Fijar aquí", "現在位置で固定"),
    T("follow", "Follow player again", "Suivre à nouveau le joueur", "Volver a seguir al jugador",
      "プレイヤーを再び追従"),
    T("panspeed", "Free-camera speed", "Vitesse de la caméra libre", "Velocidad de cámara libre",
      "自由カメラの速度"),
    T("hud", "HUD position", "Position de l’interface", "Posición de interfaz", "HUD の位置"),
    T("hudcentre", "Centre monitor / safe frame", "Écran central / cadre sûr",
      "Monitor central / zona segura", "中央の画面・安全な領域"),
    T("hudplayer", "Near the player", "Près du joueur", "Junto al jugador", "プレイヤーの近く"),
    T("hudedge", "Screen edge", "Bord de l’écran", "Borde de pantalla", "画面の端"),
    T("hidden", "Hidden", "Masquée", "Oculto", "非表示"),
    T("hudsize", "HUD size (0 = auto)", "Taille de l’interface (0 = auto)",
      "Tamaño de interfaz (0 = auto)", "HUD のサイズ (0 = 自動)"),
    T("locator", "Player locator", "Repère du joueur", "Localizador del jugador",
      "プレイヤー位置マーカー"),
    T("minimap", "Level minimap", "Mini-carte du niveau", "Minimapa del nivel",
      "ステージのミニマップ"),
    T("ghost", "Show original camera frame", "Afficher le cadre de la caméra originale",
      "Mostrar marco de cámara original", "元のカメラ枠を表示"),
    T("backgroundstyle", "Background style", "Style du décor", "Estilo del fondo",
      "背景のスタイル"),
    T("blurred", "Soft stretched landscape", "Paysage étendu et flouté",
      "Paisaje extendido y suave", "ぼかした広い風景"),
    T("sharpbg", "Sharp stretched landscape", "Paysage étendu et net", "Paisaje extendido y nítido",
      "鮮明な広い風景"),
    T("parallaxbg", "Original tiled parallax", "Parallaxe originale répétée",
      "Paralaje original repetido", "元のパララックスを繰り返す"),
    T("gradient", "Palette gradient", "Dégradé de la palette", "Degradado de la paleta",
      "パレットのグラデーション"),
    T("dark", "Quiet midnight", "Bleu nuit discret", "Azul nocturno discreto", "落ち着いた夜色"),
    T("black", "Black", "Noir", "Negro", "黒"),
    T("blur", "Background blur", "Flou du décor", "Desenfoque del fondo", "背景のぼかし"),
    T("brightness", "Background brightness", "Luminosité du décor", "Brillo del fondo",
      "背景の明るさ"),
    T("parallax", "Background follows camera", "Le décor suit la caméra",
      "El fondo sigue a la cámara", "背景がカメラに追従"),
    T("motion", "Reduced background motion", "Mouvements du décor réduits",
      "Reducir movimiento del fondo", "背景の動きを抑える"),
    T("bghelp",
      "The landscape uses the cartridge’s own scenery. Only the background softens; terrain and "
      "characters stay sharp.",
      "Le paysage utilise le décor de ta cartouche. Seul le fond est flouté ; le terrain et les "
      "personnages restent nets.",
      "El paisaje usa el escenario de tu cartucho. Solo el fondo se suaviza; terreno y personajes "
      "siguen nítidos.",
      "背景はカートリッジの風景を使います。背景だけをぼかし、地形とキャラクターは鮮明に保ちます。"),
    T("volume", "Volume", "Volume", "Volumen", "音量"),
    T("mute", "Mute game audio", "Couper le son du jeu", "Silenciar el juego",
      "ゲーム音声をミュート"),
    T("pausefocus", "Pause when unfocused", "Pause si la fenêtre est inactive",
      "Pausar al perder el foco", "非アクティブ時に一時停止"),
    T("audiohelp",
      "The app respects your system volume. Opening this menu pauses gameplay and clears queued "
      "audio.",
      "L’application respecte le volume du système. Ce menu met le jeu en pause et vide le son en "
      "attente.",
      "La aplicación respeta el volumen del sistema. Este menú pausa el juego y vacía el audio "
      "pendiente.",
      "システムの音量設定を尊重します。このメニューを開くとゲームを一時停止し、待機中の音声を消去し"
      "ます。"),
    T("keyhelp",
      "Move: arrows · Jump / action: Z, X, C · Start: Enter\nGamepad: D-pad / left stick · A, X, B "
      "· Start\nMenu: Select / View or Esc / F10 · Confirm: A / Enter · Back: B / Esc\nZoom: + / − "
      "or Ctrl + wheel · Freeze: F · Follow: R\nSave: Shift + F1…F9 · Load: F1…F9 · Fullscreen: "
      "Alt + Enter\nFree camera: WASD / right stick · Rewind: hold Backspace (enable first)",
      "Bouger : flèches · Saut / action : Z, X, C · Start : Entrée\nManette : croix / stick gauche "
      "· A, X, B · Start\nMenu : Select / View ou Échap / F10 · Valider : A / Entrée · Retour : B "
      "/ Échap\nZoom : + / − ou Ctrl + molette · Figer : F · Suivre : R\nSauver : Maj + F1…F9 · "
      "Charger : F1…F9 · Plein écran : Alt + Entrée\nCaméra libre : WASD / stick droit · Retour "
      "arrière : maintenir Retour arrière (à activer)",
      "Mover: flechas · Salto / acción: Z, X, C · Start: Intro\nMando: cruceta / stick izquierdo · "
      "A, X, B · Start\nMenú: Select / View o Esc / F10 · Confirmar: A / Intro · Volver: B / "
      "Esc\nZoom: + / − o Ctrl + rueda · Fijar: F · Seguir: R\nGuardar: Mayús + F1…F9 · Cargar: "
      "F1…F9 · Pantalla completa: Alt + Intro\nCámara libre: WASD / stick derecho · Rebobinar: "
      "Retroceso (activar antes)",
      "移動: 矢印キー ・ ジャンプ / アクション: Z, X, C ・ Start: Enter\nゲームパッド: 十字キー / "
      "左スティック ・ A, X, B ・ Start\nメニュー: Select / View または Esc / F10 ・ 決定: A / "
      "Enter ・ 戻る: B / Esc\nズーム: + / − または Ctrl + ホイール ・ 固定: F ・ 追従: R\n保存: "
      "Shift + F1…F9 ・ 読込: F1…F9 ・ 全画面: Alt + Enter\n自由カメラ: WASD / 右スティック ・ "
      "巻き戻し: Backspace を押し続ける (要設定)"),
    T("deadzone", "Stick dead zone", "Zone morte des sticks", "Zona muerta del stick",
      "スティックのデッドゾーン"),
    T("controllers", "Connected controllers", "Manettes connectées", "Mandos conectados",
      "接続中のゲームパッド"),
    T("inputhelp",
      "The first two controllers drive players 1 and 2. Mean Bean and native competition keep "
      "their original multiplayer rules.",
      "Les deux premières manettes commandent les joueurs 1 et 2. Mean Bean et les compétitions "
      "gardent leurs règles multijoueurs originales.",
      "Los dos primeros mandos controlan a los jugadores 1 y 2. Mean Bean y la competición "
      "mantienen sus reglas multijugador originales.",
      "最初の 2 台がプレイヤー 1 と 2 を操作します。Mean Bean "
      "と対戦は元のマルチプレイのルールで動作します。"),
    T("spindash", "Sonic 1 spin dash", "Spin dash dans Sonic 1", "Spin dash en Sonic 1",
      "Sonic 1 のスピンダッシュ"),
    T("dashhelp",
      "Hold Down and tap jump to charge; release Down to roll. Optional host ability for the "
      "supported Sonic 1 ROM.",
      "Maintiens Bas et appuie sur le saut pour charger ; relâche Bas pour partir en boule. "
      "Capacité optionnelle pour la ROM Sonic 1 reconnue.",
      "Mantén Abajo y pulsa salto para cargar; suelta Abajo para rodar. Habilidad opcional para la "
      "ROM compatible de Sonic 1.",
      "下を押しながらジャンプを押してチャージし、下を離すと回転して走ります。対応する Sonic 1 ROM "
      "用の追加機能です。"),
    T("spritelimit", "Remove hardware sprite limit", "Retirer la limite des sprites",
      "Quitar límite de sprites", "ハードウェアのスプライト制限を解除"),
    T("fastboot", "Skip startup logos", "Passer les logos de démarrage",
      "Saltar logotipos de inicio", "起動時のロゴをスキップ"),
    T("lockon", "Attach Sonic & Knuckles (restart game)",
      "Brancher Sonic & Knuckles (relancer le jeu)", "Acoplar Sonic & Knuckles (reiniciar juego)",
      "Sonic & Knuckles を接続 (ゲームを再起動)"),
    T("lockonhelp",
      "Sonic 3 becomes S3&K; Sonic 2 becomes Knuckles in Sonic 2; other cartridges open the "
      "original Blue Sphere lock-on feature. Import S&K first. Saves stay separate from the "
      "original game.",
      "Sonic 3 devient S3&K ; Sonic 2 devient Knuckles dans Sonic 2 ; les autres cartouches "
      "ouvrent Blue Sphere. Importe S&K d’abord. Les sauvegardes restent séparées du jeu "
      "d’origine.",
      "Sonic 3 se convierte en S3&K; Sonic 2 en Knuckles in Sonic 2; otros cartuchos abren Blue "
      "Sphere. Importa S&K primero. Las partidas se guardan por separado.",
      "Sonic 3 は S3&K、Sonic 2 は Knuckles in Sonic 2 になります。他のカートリッジは元の Blue "
      "Sphere ロックオン機能を開きます。先に S&K "
      "を読み込んでください。元のゲームとは別にセーブします。"),
    T("rewind", "Enable rewind (uses memory)", "Activer le retour arrière (mémoire)",
      "Activar rebobinado (usa memoria)", "巻き戻しを有効化 (メモリを使用)"),
    T("assist", "Sonic 1 damage protection", "Protection contre les dégâts dans Sonic 1",
      "Protección de daño en Sonic 1", "Sonic 1 のダメージ保護"),
    T("air", "Sonic 1 unlimited air", "Air illimité dans Sonic 1", "Aire ilimitado en Sonic 1",
      "Sonic 1 の空気を無制限に"),
    T("region", "Console region (restart game)", "Région de la console (relancer le jeu)",
      "Región de consola (reiniciar juego)", "本体の地域 (ゲームを再起動)"),
    T("accuracy",
      "Classic simulation stays at the cartridge’s speed. Far-away scenery and rings are visible; "
      "enemies become active near the original camera. Special stages and native competition use "
      "original framing.",
      "La simulation garde la vitesse de la cartouche. Le décor et les anneaux lointains sont "
      "visibles ; les ennemis s’activent près de la caméra originale. Les étapes spéciales et les "
      "compétitions gardent leur cadre original.",
      "La simulación conserva la velocidad del cartucho. Se ven escenarios y anillos lejanos; los "
      "enemigos se activan cerca de la cámara original. Las fases especiales y la competición usan "
      "su encuadre original.",
      "シミュレーションはカートリッジの速度を維持します。遠くの地形とリングを表示し、敵は元のカメラ"
      "付近で動作します。スペシャルステージと対戦は元の画面で表示します。"),
    T("slot", "Save slot", "Emplacement", "Ranura de guardado", "セーブスロット"),
    T("save", "Save now", "Sauvegarder", "Guardar ahora", "今すぐ保存"),
    T("load", "Load slot", "Charger", "Cargar ranura", "スロットを読み込む"),
    T("autosave", "Auto-resume on next launch", "Reprise automatique au prochain lancement",
      "Continuar al abrir de nuevo", "次回起動時に自動で再開"),
    T("reset", "Reset current game", "Redémarrer le jeu", "Reiniciar juego actual",
      "現在のゲームをリセット"),
    T("savehelp",
      "Each game has nine state slots and its own cartridge save. Previous files are kept as .bak. "
      "Slots reject a different ROM or damaged data.",
      "Chaque jeu a neuf emplacements et sa sauvegarde de cartouche. Les anciennes versions "
      "restent en .bak. Une ROM différente ou un fichier abîmé est refusé.",
      "Cada juego tiene nueve ranuras y su guardado de cartucho. Se conservan archivos .bak. Se "
      "rechazan otras ROMs o datos dañados.",
      "各ゲームに 9 個のステートスロットとカートリッジのセーブがあります。以前のファイルは .bak "
      "で保存し、異なる ROM や破損データを拒否します。"),
    T("capture", "Save screenshot", "Capturer l’écran", "Guardar captura",
      "スクリーンショットを保存"),
    T("help", "Keep menu hint visible", "Garder l’indication du menu",
      "Mantener visible la ayuda del menú", "メニューの操作案内を表示"),
    T("defaults", "Restore default options", "Rétablir les options par défaut",
      "Restaurar opciones por defecto", "初期設定に戻す"),
    T("browse", "Choose a file or folder", "Choisir un fichier ou dossier",
      "Elegir archivo o carpeta", "ファイルかフォルダーを選ぶ"),
    T("path", "Location", "Emplacement", "Ubicación", "場所"),
    T("up", "Parent folder", "Dossier parent", "Carpeta superior", "親フォルダー"),
    T("open", "Open", "Ouvrir", "Abrir", "開く"),
    T("thisfolder", "Import this folder", "Importer ce dossier", "Importar esta carpeta",
      "このフォルダーを読み込む"),
    T("romguide",
      "Use your own cartridge dump or an authorised backup. Search for “dump my Sega Genesis "
      "cartridge” and the exact game title to learn about compatible cartridge readers. Do not "
      "search for pirate ROM download sites. Owning a cartridge does not grant every possible use "
      "in every jurisdiction. This app supplies no game-download links.",
      "Utilise une extraction de ta cartouche ou une copie autorisée. Recherche « dump my Sega "
      "Genesis cartridge » et le titre exact pour connaître les lecteurs compatibles. Évite les "
      "sites de ROMs piratées. Posséder une cartouche n’autorise pas tous les usages dans tous les "
      "pays. Cette application ne fournit aucun lien de téléchargement des jeux.",
      "Usa un volcado de tu cartucho o una copia autorizada. Busca “dump my Sega Genesis "
      "cartridge” y el título exacto para conocer lectores compatibles. Evita sitios de descargas "
      "piratas. Poseer un cartucho no autoriza todos los usos en todos los países. Esta aplicación "
      "no enlaza descargas de juegos.",
      "自分のカートリッジから吸い出したデータ、または利用を許可されたコピーを使ってください。「dump"
      " my Sega Genesis cartridge」と正確なゲーム名で対応する読み取り機器を調べられます。海賊版 "
      "ROM "
      "のサイトは案内しません。カートリッジの所有だけで、すべての国ですべての利用が許可されるわけで"
      "はありません。ゲームのダウンロードリンクは提供しません。"),
    T("romformats",
      "Supported files: .bin, .md, .gen, .smd and .zip. Keep your originals: import records a path "
      "and reads the ROM locally.",
      "Formats : .bin, .md, .gen, .smd et .zip. Garde tes originaux : l’import mémorise le chemin "
      "et lit la ROM localement.",
      "Formatos: .bin, .md, .gen, .smd y .zip. Conserva los originales: la importación registra la "
      "ruta y lee la ROM localmente.",
      "対応形式: .bin, .md, .gen, .smd, .zip。読み込みはパスを記録してローカルで ROM "
      "を読むため、元のファイルをそのまま保管できます。"),
    T("romrevision",
      "These checksums enable the expanded world view. Other valid Mega Drive ROMs can use "
      "original framing. The name alone is not enough to identify a revision.",
      "Ces empreintes activent la vue étendue. Les autres ROMs Mega Drive valides peuvent utiliser "
      "le cadre original. Le nom seul ne permet pas de connaître la révision.",
      "Estas sumas activan la vista ampliada. Otras ROMs válidas de Mega Drive pueden usar el "
      "encuadre original. El nombre no basta para identificar la revisión.",
      "以下のチェックサムが拡張ワールド表示に対応します。他の有効な Mega Drive ROM "
      "は元の画面で動作できます。ファイル名だけでは版を判定できません。"),
    T("credits",
      "Independent, noncommercial fan software. Not affiliated with or endorsed by SEGA. Game "
      "names and graphics belong to their respective rights holders. Runtime: Genesis Plus GX. "
      "Interface: SDL2, Dear ImGui, Noto Sans JP. No ROM, music or reusable extracted game artwork "
      "is bundled.",
      "Logiciel de fan indépendant et non commercial, sans affiliation ni approbation de SEGA. Les "
      "noms et graphismes des jeux appartiennent à leurs ayants droit. Moteur : Genesis Plus GX. "
      "Interface : SDL2, Dear ImGui, Noto Sans JP. Aucune ROM, musique ni image extraite "
      "réutilisable n’est fournie.",
      "Software de fans independiente y no comercial, sin afiliación ni respaldo de SEGA. Los "
      "nombres y gráficos pertenecen a sus titulares. Motor: Genesis Plus GX. Interfaz: SDL2, Dear "
      "ImGui, Noto Sans JP. No se incluyen ROMs, música ni gráficos extraídos reutilizables.",
      "独立した非商用のファンソフトウェアです。SEGA "
      "の公式製品でも、承認された製品でもありません。ゲーム名や画像の権利は各権利者に帰属します。エ"
      "ンジン: Genesis Plus GX。UI: SDL2、Dear ImGui、Noto Sans "
      "JP。ROM、音楽、再利用可能な抽出画像は同梱しません。"),
    T("limitations",
      "World view is experimental. Animated terrain, unusual scrolling and transitions can require "
      "Original framing. Full-game completion and every physical controller/GPU are not certified. "
      "Choose Original framing at any time to preserve the native display.",
      "La vue étendue est expérimentale. Les terrains animés, défilements particuliers et "
      "transitions peuvent nécessiter le cadre original. Tous les parcours, manettes et GPU ne "
      "sont pas certifiés. Le cadre original reste disponible à tout moment.",
      "La vista ampliada es experimental. Terrenos animados, desplazamientos especiales y "
      "transiciones pueden necesitar el encuadre original. No se certifica la finalización "
      "completa ni cada mando o GPU. El encuadre original está siempre disponible.",
      "拡張表示は実験的です。地形のアニメーション、特殊なスクロールや切り替えでは元の画面が必要な場"
      "合があります。全ゲームの完全攻略や、すべての実機ゲームパッド・GPU "
      "を認証したものではありません。いつでも元の画面に切り替えられます。"),
    T("score", "SCORE", "SCORE", "PUNTOS", "スコア"),
    T("rings", "RINGS", "ANNEAUX", "ANILLOS", "リング"),
    T("time", "TIME", "TEMPS", "TIEMPO", "タイム"),
    T("lives", "LIVES", "VIES", "VIDAS", "残機"),
    T("charge", "SPIN DASH", "SPIN DASH", "SPIN DASH", "スピンダッシュ"),
    T("notice", "Notice", "Information", "Aviso", "お知らせ"),
    T("error", "Could not complete this action", "Cette action n’a pas pu aboutir",
      "No se pudo completar la acción", "操作を完了できませんでした"),
    T("imported", "Cartridges imported", "Cartouches importées", "Cartuchos importados",
      "カートリッジを読み込みました"),
    T("noimport", "No supported cartridge found", "Aucune cartouche compatible trouvée",
      "No se encontró un cartucho compatible", "対応するカートリッジがありません"),
    T("hotkeytip", "Use the tabs for more options. Changes are saved automatically.",
      "Les onglets contiennent d’autres options. Les changements sont sauvegardés automatiquement.",
      "Usa las pestañas para más opciones. Los cambios se guardan automáticamente.",
      "タブで他の設定を開けます。変更は自動的に保存されます。"),
};
const char *tr(const char *key, int language) {
    for (auto &t : texts)
        if (std::strcmp(t.key, key) == 0)
            return t.text[language >= 0 && language < 4 ? language : 0];
    return key;
}
int detectLanguage(int requested) {
    if (requested >= 0 && requested < 4)
        return requested;
    int result = 0;
    SDL_Locale *locales = SDL_GetPreferredLocales();
    if (locales) {
        if (locales[0].language) {
            auto p = locales[0].language;
            if (!std::strcmp(p, "fr"))
                result = 1;
            else if (!std::strcmp(p, "es"))
                result = 2;
            else if (!std::strcmp(p, "ja"))
                result = 3;
        }
        SDL_free(locales);
    }
    return result;
}
} // namespace scu
