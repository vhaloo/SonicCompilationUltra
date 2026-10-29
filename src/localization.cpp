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
    T("questworldbackdrop", "Wrap the game's backdrop around the VR room",
      "Étendre le décor sur 360° en VR", "Fondo del juego a 360° en RV",
      "VRの周囲360度に背景を表示"),
    T("desktoppresentation", "Desktop frame", "Cadre sur PC", "Marco de escritorio",
      "デスクトップのフレーム"),
    T("desktopplain", "Full canvas", "Canevas entier", "Lienzo completo", "キャンバス全体"),
    T("desktopcabinet", "Pinball cabinet frame", "Cadre de flipper", "Marco de pinball",
      "ピンボール筐体フレーム"),
    T("desktopcrt", "CRT and surrounding level", "CRT et niveau autour", "CRT y nivel alrededor",
      "ブラウン管と周囲のステージ"),
    T("desktopframescale", "Frame size", "Taille du cadre", "Tamaño del marco", "フレームの大きさ"),
    T("desktopcabinetangle", "Cabinet perspective", "Perspective du flipper",
      "Perspectiva del mueble", "筐体の遠近感"),
    T("desktopcrtstrength", "Desktop CRT strength", "Intensité CRT sur PC",
      "Intensidad CRT de escritorio", "デスクトップCRT効果の強さ"),
    T("desktopcrtmask", "Subtle desktop phosphor mask", "Masque de phosphore discret sur PC",
      "Máscara de fósforo sutil de escritorio", "デスクトップの弱い蛍光体マスク"),
    T("desktopsurround", "Extend the level behind the frame", "Étendre le niveau derrière le cadre",
      "Extender el nivel detrás del marco", "フレームの背後にステージを展開"),
    T("questpinball", "3D full-board pinball", "Flipper 3D, plateau entier", "Pinball 3D completo",
      "全盤面3Dピンボール"),
    T("questcrtroom", "3D CRT + surrounding world", "CRT 3D et monde autour",
      "CRT 3D y mundo alrededor", "3Dブラウン管と周囲の世界"),
    T("questcurved", "Curved level gallery", "Galerie de niveau incurvée", "Galería de nivel curva",
      "湾曲ステージギャラリー"),
    T("questspinballauto", "Use the 3D cabinet automatically for Spinball",
      "Flipper 3D automatique pour Spinball", "Usar mueble 3D automáticamente en Spinball",
      "Spinballを自動で3D筐体に表示"),
    T("questscale", "Physical scale", "Échelle physique", "Escala física", "実寸スケール"),
    T("questhalf", "Half size", "Taille ÷ 2", "Mitad de tamaño", "半分のサイズ"),
    T("questdouble", "Double size", "Taille × 2", "Doble de tamaño", "2倍のサイズ"),
    T("questscalereset", "Reset to 100%", "Revenir à 100 %", "Volver al 100 %", "100%に戻す"),
    T("questresizespeed", "Grip + stick resize speed", "Vitesse de zoom prises + stick",
      "Velocidad de agarres y palanca", "グリップとスティックの拡縮速度"),
    T("questresizehint",
      "Hold both grips and spread your hands to resize, even while placement is locked. Regrip to "
      "keep scaling, or move the right stick up/down while gripping. Y opens options; R3 brings "
      "the menu back nearby.",
      "Maintiens les deux prises et écarte les mains pour agrandir, même si le placement est "
      "verrouillé. Reprends les prises pour continuer, ou utilise le stick droit haut/bas en les "
      "maintenant. Y : options ; R3 : menu près de toi.",
      "Mantén ambos agarres y separa las manos para cambiar el tamaño, incluso con posición "
      "bloqueada. Suelta y vuelve a agarrar o usa la palanca derecha arriba/abajo. Y: opciones; "
      "R3: acercar menú.",
      "両グリップを押して手を広げると、位置固定中でも拡大できます。握り直すか右スティック上下で続け"
      "て拡縮。Yで設定、R3でメニューを近くに戻します。"),
    T("questcabinetwidth", "Pinball playfield width", "Largeur du plateau de flipper",
      "Anchura del tablero de pinball", "ピンボール盤面の幅"),
    T("questcabinetheight", "Pinball front height", "Hauteur avant du flipper",
      "Altura frontal del pinball", "筐体前面の高さ"),
    T("questcabinettilt", "Playfield slope", "Inclinaison du plateau", "Inclinación del tablero",
      "盤面の傾斜"),
    T("questglass", "Glass reflections", "Reflets du verre", "Reflejos del cristal",
      "ガラスの反射"),
    T("questpinballnote",
      "All four supported Spinball boards keep their complete terrain and original gameplay. The "
      "cabinet adds 3D furniture, rails, glass and a native backglass view. Objects still follow "
      "the cartridge's activation range.",
      "Les quatre plateaux compatibles conservent leur terrain entier et leur gameplay original. "
      "Le meuble ajoute structure 3D, rails, verre et vue native sur le fronton. Les objets "
      "suivent toujours la portée d’activation de la cartouche.",
      "Los cuatro tableros compatibles conservan todo el terreno y la jugabilidad original. El "
      "mueble añade estructura 3D, raíles, cristal y pantalla nativa. Los objetos respetan el "
      "alcance de activación del cartucho.",
      "対応する4盤面の全地形と元のゲーム動作を保持します。3D筐体、レール、ガラス、元の画面を映すバ"
      "ックボードを追加。オブジェクトの動作範囲はカートリッジに従います。"),
    T("questtvwidth", "CRT picture width", "Largeur de l’image CRT", "Anchura de imagen CRT",
      "ブラウン管の映像幅"),
    T("questcrt", "CRT picture preset", "Profil d’image CRT", "Perfil de imagen CRT",
      "ブラウン管プリセット"),
    T("questcrtoff", "Off", "Désactivé", "Desactivado", "オフ"),
    T("questcrtclean", "Clean monitor", "Moniteur net", "Monitor limpio", "クリアモニター"),
    T("questcrtconsumer", "Living-room CRT", "Téléviseur de salon", "Televisor doméstico",
      "家庭用ブラウン管"),
    T("questcrtarcade", "Arcade tube", "Tube d’arcade", "Tubo de arcade", "アーケード管"),
    T("questcrtmask", "Phosphor mask", "Masque de phosphore", "Máscara de fósforo", "蛍光体マスク"),
    T("questmasklight", "Subtle RGB", "RVB discret", "RGB sutil", "弱いRGB"),
    T("questmaskstrong", "Strong RGB", "RVB marqué", "RGB intenso", "強いRGB"),
    T("questcrtstrength", "CRT effect strength", "Intensité de l’effet CRT",
      "Intensidad del efecto CRT", "ブラウン管効果の強さ"),
    T("questcrtcurve", "Tube curvature", "Courbure du tube", "Curvatura del tubo", "画面の湾曲"),
    T("questsurround", "Expand the level around the CRT", "Étendre le niveau autour du CRT",
      "Extender el nivel alrededor del CRT", "ブラウン管の周囲にステージを展開"),
    T("questsurroundbrightness", "Surrounding level brightness", "Luminosité du niveau autour",
      "Brillo del nivel alrededor", "周囲のステージの明るさ"),
    T("questcurve", "Gallery wrap angle", "Angle de la galerie", "Ángulo de la galería",
      "ギャラリーの角度"),
    T("questenvironment", "VR surroundings", "Environnement VR", "Entorno de RV", "VRの環境"),
    T("questvoid", "Dark void", "Fond sombre", "Vacío oscuro", "暗い空間"),
    T("questlounge", "Quiet studio floor", "Sol de studio discret", "Suelo de estudio discreto",
      "落ち着いたスタジオの床"),
    T("queststereonote",
      "3D furniture uses stereo rendering; options stay on a separate sharp panel. AR shows your "
      "room instead of the studio. The preview pattern contains no game assets.",
      "Le mobilier est rendu en stéréo ; les options restent sur un panneau net séparé. En AR, ta "
      "pièce remplace le studio. Le motif d’aperçu ne contient aucun asset de jeu.",
      "Los muebles se renderizan en estéreo; las opciones conservan un panel nítido. RA muestra tu "
      "habitación en lugar del estudio. El patrón de vista previa no contiene recursos del juego.",
      "家具は立体描画し、設定は別の鮮明なパネルに表示。ARではスタジオの代わりに実際の部屋が見えます"
      "。プレビュー模様にゲーム素材は含まれません。"),
    T("questpinballpreset", "Giant pinball preset", "Profil flipper géant",
      "Perfil de pinball gigante", "巨大ピンボール設定"),
    T("questcrtpreset", "CRT world preset", "Profil monde CRT", "Perfil de mundo CRT",
      "ブラウン管ワールド設定"),
    T("questcurvedpreset", "Curved gallery preset", "Profil galerie incurvée",
      "Perfil de galería curva", "湾曲ギャラリー設定"),
    T("questmode", "Presentation mode", "Mode de présentation", "Modo de presentación",
      "表示モード"),
    T("questscreenmode", "Spatial screen", "Écran spatial", "Pantalla espacial", "空間スクリーン"),
    T("questwallfit", "Fit a scanned wall", "Ajuster au mur scanné", "Ajustar a la pared",
      "壁の範囲に合わせる"),
    T("questmural", "Two-metre level mural", "Niveau mural de deux mètres",
      "Mural de nivel de dos metros", "高さ2メートルのステージ壁画"),
    T("questflooratlas", "Floor atlas", "Atlas au sol", "Atlas en el suelo", "床のステージ地図"),
    T("questwallheight", "Mural height from the floor", "Hauteur du niveau depuis le sol",
      "Altura del mural desde el suelo", "床からの壁画の高さ"),
    T("questwallgap", "Gap above the floor", "Écart au-dessus du sol", "Separación del suelo",
      "床との間隔"),
    T("questwalldistance", "Manual placement distance", "Distance du placement manuel",
      "Distancia de colocación manual", "手動配置の距離"),
    T("questautowall", "Find a scanned wall automatically", "Trouver automatiquement un mur scanné",
      "Buscar pared escaneada automáticamente", "スキャン済みの壁を自動選択"),
    T("questwalllock", "Lock wall placement", "Verrouiller le placement du mur",
      "Bloquear la colocación", "壁の配置を固定"),
    T("questpauselost", "Pause when wall tracking is lost", "Pause si le suivi du mur est perdu",
      "Pausar al perder el seguimiento", "壁の追跡が失われたら一時停止"),
    T("questcompanion", "Keep a companion menu near me", "Garder les options près de moi",
      "Mantener el menú cerca", "操作メニューを近くに表示"),
    T("questmenudistance", "Companion menu distance", "Distance du menu de proximité",
      "Distancia del menú cercano", "操作メニューまでの距離"),
    T("questwalldetail", "Mural detail budget", "Détail du niveau mural", "Detalle del mural",
      "壁画の描画品質"),
    T("questdetailfast", "Fast", "Rapide", "Rápido", "軽量"),
    T("questdetailbalanced", "Balanced", "Équilibré", "Equilibrado", "バランス"),
    T("questdetailsharp", "Sharp", "Détaillé", "Nítido", "高精細"),
    T("questadaptive", "Adaptive mural resolution", "Résolution adaptative du niveau mural",
      "Resolución adaptable del mural", "壁画の解像度を自動調整"),
    T("questplacewall", "Place virtual wall in front of me", "Placer le mur virtuel devant moi",
      "Colocar pared virtual delante", "正面に仮想の壁を配置"),
    T("questfindwall", "Auto place on a scanned wall", "Placer sur un mur scanné",
      "Colocar en pared escaneada", "スキャン済みの壁に配置"),
    T("questnextwall", "Next wall", "Mur suivant", "Siguiente pared", "次の壁"),
    T("questscanroom", "Capture / update room", "Capturer / actualiser la pièce",
      "Capturar / actualizar habitación", "部屋をスキャン・更新"),
    T("questcalibratefloor", "Use left controller height as floor",
      "Utiliser la manette gauche comme niveau du sol",
      "Usar altura del mando izquierdo como suelo", "左コントローラーの高さを床に設定"),
    T("questsavewall", "Save wall placement locally", "Enregistrer le placement localement",
      "Guardar colocación localmente", "壁の配置を本体に保存"),
    T("questrestorewall", "Restore saved placement", "Restaurer le placement enregistré",
      "Restaurar colocación guardada", "保存した配置を復元"),
    T("questrefresh", "Headset refresh preference", "Fréquence souhaitée du casque",
      "Frecuencia preferida del visor", "ヘッドセットの更新頻度"),
    T("questrefreshmax", "Maximum available (default)", "Maximum disponible (par défaut)",
      "Máxima disponible (predeterminada)", "利用可能な最大値（初期設定）"),
    T("questrefreshsystem", "System preference", "Choix du système", "Preferencia del sistema",
      "システムに任せる"),
    T("questrefreshruntime", "Headset: %.0f Hz · requested: %.0f Hz",
      "Casque : %.0f Hz · demandé : %.0f Hz", "Visor: %.0f Hz · solicitado: %.0f Hz",
      "ヘッドセット：%.0f Hz · 要求値：%.0f Hz"),
    T("questrefreshsystemactive", "Headset: %.0f Hz · request: system",
      "Casque : %.0f Hz · demande : système", "Visor: %.0f Hz · solicitud: sistema",
      "ヘッドセット：%.0f Hz · システムに任せる"),
    T("questrefreshunknown", "Runtime refresh information is not available yet.",
      "La fréquence du casque n’est pas encore disponible.",
      "La frecuencia del visor aún no está disponible.",
      "実行環境の更新頻度はまだ取得できません。"),
    T("questrefreshdenied",
      "The runtime declined the refresh request; its system preference remains active.",
      "Le casque a refusé la fréquence ; le choix du système reste actif.",
      "El visor rechazó la frecuencia; se conserva la del sistema.",
      "更新頻度の要求は受け付けられませんでした。システムの設定を使います。"),
    T("questpersonal", "Personal head-following screen", "Écran personnel qui suit la tête",
      "Pantalla personal que sigue la cabeza", "頭に追従する個人スクリーン"),
    T("questwallmanual", "Placement: manual virtual plane", "Placement : plan virtuel manuel",
      "Colocación: plano virtual manual", "配置：手動の仮想平面"),
    T("questwallscanned", "Placement: tracked scanned wall", "Placement : mur scanné suivi",
      "Colocación: pared escaneada con seguimiento", "配置：スキャンした壁に追従"),
    T("questwallanchored", "Placement: saved local anchor", "Placement : ancrage local enregistré",
      "Colocación: anclaje local guardado", "配置：本体に保存したアンカー"),
    T("questwalltrackinglost",
      "Wall tracking lost. The wall is hidden; restore or place it again from this menu.",
      "Suivi du mur perdu. Le niveau est masqué ; restaure ou replace le mur depuis ce menu.",
      "Seguimiento perdido. El mural está oculto; restaura o colócalo de nuevo desde el menú.",
      "壁の追跡が失われたため非表示にしました。メニューで復元するか配置し直してください。"),
    T("questwalllockednotice",
      "Wall stays fixed. Y opens options; right-stick click recenters their companion panel.",
      "Le mur reste fixe. Y ouvre les options ; le clic du stick droit recentre leur menu.",
      "La pared sigue fija. Y abre opciones; pulsa la palanca derecha para acercar su panel.",
      "壁は固定されています。Yで設定を開き、右スティック押下でメニューを正面に戻します。"),
    T("questfloorknown", "Floor: tracked or manually calibrated",
      "Sol : suivi ou calibré manuellement", "Suelo: seguido o calibrado manualmente",
      "床：追跡または手動で設定済み"),
    T("questfloorestimated", "Floor: standing-height estimate. Calibrate if needed.",
      "Sol : hauteur debout estimée. Calibre au besoin.",
      "Suelo: altura de pie estimada. Calibra si es necesario.",
      "床：立った状態の推定値です。必要なら手動で調整してください。"),
    T("questwallnote",
      "Fixed stage framing overrides normal camera choices until Spatial screen is restored. "
      "Without a usable room model, place a virtual wall manually. A mural can extend beyond the "
      "real wall.",
      "Le cadrage fixe remplace la caméra normale jusqu’au retour en Écran spatial. Sans modèle de "
      "pièce utilisable, place le mur manuellement. Le niveau peut dépasser le vrai mur.",
      "El encuadre fijo reemplaza la cámara normal hasta volver a Pantalla espacial. Sin modelo de "
      "habitación, coloca la pared manualmente. El mural puede superar la pared real.",
      "空間スクリーンに戻すまでステージ全体の固定表示を使います。部屋のモデルがない場合は仮想の壁を"
      "手動配置してください。壁画は実際の壁より広くできます。"),
    T("questwallhint", "Y Options · X Close-up · LT Rewind", "Y Options · X Gros plan · LT Rewind",
      "Y Opciones · X Detalle · LT Rebobinar", "Y 設定 · X 拡大 · LT 巻き戻し"),
    T("questsceneidle", "Scene access is optional; manual placement is available.",
      "L’accès à la pièce est facultatif ; le placement manuel reste disponible.",
      "El acceso a la habitación es opcional; puedes colocarla manualmente.",
      "部屋データへのアクセスは任意です。手動でも配置できます。"),
    T("questsceneloading", "Loading saved room surfaces…", "Chargement des surfaces de la pièce…",
      "Cargando superficies guardadas…", "保存済みの部屋の面を読み込み中…"),
    T("questrestoringwall", "Locating the saved wall anchor…", "Recherche de l’ancrage enregistré…",
      "Localizando el anclaje guardado…", "保存した壁のアンカーを探索中…"),
    T("questnosavedwall", "No saved wall anchor was located.",
      "Aucun ancrage de mur enregistré n’a été localisé.",
      "No se localizó ningún anclaje de pared guardado.", "保存した壁のアンカーが見つかりません。"),
    T("questsceneunavailable",
      "Room data is unavailable. Check spatial permission or place a virtual wall manually.",
      "Données de pièce indisponibles. Vérifie l’autorisation spatiale ou place le mur "
      "manuellement.",
      "Datos de habitación no disponibles. Revisa el permiso espacial o coloca la pared "
      "manualmente.",
      "部屋データを利用できません。空間データの権限を確認するか、仮想の壁を手動配置してください。"),
    T("questscenenone", "No saved room surfaces found. Capture the room or use manual placement.",
      "Aucune surface de pièce trouvée. Capture la pièce ou utilise le placement manuel.",
      "No hay superficies guardadas. Captura la habitación o usa colocación manual.",
      "部屋の面が見つかりません。部屋をスキャンするか手動で配置してください。"),
    T("questsceneready", "Saved room surfaces loaded. Only nearby, locatable walls are used.",
      "Surfaces chargées. Seuls les murs proches et localisables sont proposés.",
      "Superficies cargadas. Solo se usan paredes cercanas y localizables.",
      "部屋の面を読み込みました。近くで追跡できる壁だけを使います。"),
    T("questscenecapture", "Complete Meta’s room capture, then return here.",
      "Termine la capture de pièce de Meta, puis reviens ici.",
      "Completa la captura de habitación de Meta y vuelve aquí.",
      "Metaの部屋スキャンを完了してから戻ってください。"),
    T("questwallsaving", "Saving the wall anchor on this headset…",
      "Enregistrement de l’ancrage sur ce casque…", "Guardando el anclaje en este visor…",
      "このヘッドセットに壁のアンカーを保存中…"),
    T("questwallrestored", "Saved wall anchor restored.", "Ancrage du mur restauré.",
      "Anclaje de pared restaurado.", "保存した壁のアンカーを復元しました。"),
    T("questwallsaved", "Wall anchor saved locally.", "Ancrage du mur enregistré localement.",
      "Anclaje de pared guardado localmente.", "壁のアンカーを本体に保存しました。"),
    T("questsavefailed", "Wall anchor could not be saved. The previous record is kept.",
      "L’ancrage n’a pas pu être enregistré. Le précédent est conservé.",
      "No se pudo guardar el anclaje. Se conserva el registro anterior.",
      "壁のアンカーを保存できませんでした。以前の記録は保持しています。"),
    T("questtab", "Quest VR / AR", "Quest VR / AR", "Quest VR / RA", "Quest VR / AR"),
    T("questguidebutton", "Setting guide · X", "Guide du réglage · X", "Guía del ajuste · X",
      "設定ガイド · X"),
    T("questguidehint",
      "Point at or focus a setting for its illustrated explanation. X opens this guide; B or Y "
      "closes it first.",
      "Pointe ou sélectionne un réglage pour son explication illustrée. X ouvre ce guide ; B ou Y "
      "le ferme en premier.",
      "Apunta o selecciona un ajuste para ver su explicación. X abre esta guía; B o Y la cierra "
      "primero.",
      "設定を指すか選ぶと図付き説明が表示されます。Xでこのガイドを開き、BまたはYで閉じます。"),
    T("questtrigger", "left trigger", "gâchette gauche", "gatillo izquierdo", "左トリガー"),
    T("questintro",
      "Choose a giant fixed mural, a complete-board 3D pinball cabinet, a curved gallery or a 3D "
      "CRT with the level around it. Use both grips to resize. Experimental XR preview.",
      "Choisis un mural géant fixe, un flipper 3D avec plateau entier, une galerie incurvée ou un "
      "CRT 3D entouré du niveau. Les deux prises redimensionnent. Aperçu XR expérimental.",
      "Elige mural gigante fijo, pinball 3D completo, galería curva o CRT 3D rodeado del nivel. "
      "Ambos agarres cambian el tamaño. Vista previa XR experimental.",
      "固定された巨大壁画、全盤面3Dピンボール、湾曲ギャラリー、ステージに囲まれた3Dブラウン管を選べ"
      "ます。両グリップで拡縮。実験的なXRプレビュー版です。"),
    T("questar", "Passthrough AR · see the room", "AR avec passthrough · voir la pièce",
      "RA con passthrough · ver la habitación", "パススルーAR · 部屋を表示"),
    T("questarunavailable", "Passthrough is unavailable in this session. VR remains available.",
      "Le passthrough est indisponible dans cette session. La VR reste accessible.",
      "El passthrough no está disponible en esta sesión. La RV sigue disponible.",
      "このセッションではパススルーを利用できません。VRは利用できます。"),
    T("questanchor", "Screen anchoring", "Ancrage de l’écran", "Anclaje de pantalla",
      "スクリーンの固定"),
    T("questroom", "Pinned in the room", "Fixé dans la pièce", "Fija en la habitación",
      "部屋に固定"),
    T("questhead", "Follow the head", "Suivre la tête", "Seguir la cabeza", "頭に追従"),
    T("questwidth", "Physical screen width", "Largeur physique de l’écran",
      "Ancho físico de pantalla", "スクリーンの幅"),
    T("questdistance", "Viewing distance", "Distance de visionnement", "Distancia de visión",
      "表示距離"),
    T("questheight", "Height relative to the eyes", "Hauteur par rapport aux yeux",
      "Altura respecto a los ojos", "目の高さからの位置"),
    T("questtilt", "Tabletop tilt", "Inclinaison de la table", "Inclinación de mesa",
      "テーブルの傾き"),
    T("questresolution", "Spatial screen resolution", "Résolution de l’écran spatial",
      "Resolución de la pantalla espacial", "空間スクリーンの解像度"),
    T("questeco", "Lightweight · 1280 × 720", "Léger · 1280 × 720", "Ligero · 1280 × 720",
      "軽量 · 1280 × 720"),
    T("questbalanced", "Balanced · 1920 × 1080", "Équilibré · 1920 × 1080",
      "Equilibrado · 1920 × 1080", "標準 · 1920 × 1080"),
    T("questsharp", "Sharp · 2560 × 1440", "Net · 2560 × 1440", "Nítido · 2560 × 1440",
      "高精細 · 2560 × 1440"),
    T("questwide", "Ultra-wide 48:9 · 3072 × 576", "Ultra-large 48:9 · 3072 × 576",
      "Ultraancha 48:9 · 3072 × 576", "超横長48:9 · 3072 × 576"),
    T("questtouch", "Use Quest Touch controls", "Utiliser les contrôleurs Quest Touch",
      "Usar mandos Quest Touch", "Quest Touchコントローラーを使用"),
    T("questhaptics", "Touch vibration strength", "Intensité des vibrations Touch",
      "Intensidad de vibración Touch", "Touchの振動の強さ"),
    T("questflip", "Flip the screen image vertically", "Retourner l’image verticalement",
      "Invertir imagen verticalmente", "画面を上下反転"),
    T("questrecenter", "Recenter screen · right stick click", "Recentrer · clic du stick droit",
      "Recentrar · clic de palanca derecha", "画面を中央へ · 右スティック押下"),
    T("questcinema", "Cinema layout", "Disposition cinéma", "Disposición de cine", "シネマ配置"),
    T("questtabletop", "Tabletop whole-stage layout", "Table avec niveau entier",
      "Mesa con nivel completo", "ステージ全体をテーブル表示"),
    T("questgrab",
      "Hold both grip buttons to move and scale a room-pinned screen. Release to pin it. Point the "
      "right controller at the menu and squeeze its trigger to click; the right stick scrolls.",
      "Maintiens les deux boutons de prise pour déplacer et redimensionner l’écran fixé. Relâche "
      "pour le fixer. Pointe le contrôleur droit sur le menu et presse sa gâchette pour cliquer ; "
      "le stick droit défile.",
      "Mantén ambos botones de agarre para mover y escalar la pantalla fija. Suelta para fijarla. "
      "Apunta al menú con el mando derecho y pulsa su gatillo; la palanca derecha desplaza.",
      "両方のグリップを押して固定スクリーンを移動・拡大縮小し、離すと固定します。右コントローラーで"
      "メニューを指し、トリガーで選択。右スティックでスクロールします。"),
    T("questqualitynote",
      "Lightweight reduces battery and rendering load. A larger virtual canvas costs more GPU "
      "work. Pixel 1:1 refers to game pixels in this canvas; headset optics and the compositor "
      "resample the final image. For a Bluetooth gamepad, switch Touch controls off.",
      "Léger réduit la charge et la consommation. Un canevas plus grand sollicite davantage le "
      "GPU. Le 1:1 concerne les pixels du jeu dans ce canevas ; les optiques et le compositeur "
      "rééchantillonnent l’image. Pour une manette Bluetooth, désactive les contrôleurs Touch.",
      "Ligero reduce carga y consumo. Un lienzo mayor requiere más GPU. El 1:1 se refiere a "
      "píxeles del juego en el lienzo; óptica y compositor remuestrean la imagen final. Para un "
      "mando Bluetooth, desactiva los mandos Touch.",
      "軽量設定は処理と消費電力を抑えます。キャンバスが大きいほどGPU負荷が増えます。1:"
      "1はキャンバス内の画素を指し、光学系と合成処理で最終画像は再標本化されます。Bluetoothゲームパ"
      "ッドではTouch操作を無効にしてください。"),
    T("questhint", "Options · Y", "Options · Y", "Opciones · Y", "設定 · Y"),
    T("questnav", "A: confirm · B: back · grips: tabs · right stick: scroll · X: guide · Y: menu",
      "A : valider · B : retour · prises : onglets · stick droit : défiler · X : guide · Y : menu",
      "A: aceptar · B: volver · agarres: pestañas · palanca derecha: desplazar · X: guía · Y: menú",
      "A:決定 · B:戻る · グリップ:タブ · 右スティック:スクロール · X:ガイド · Y:メニュー"),
    T("hintshortcuts", "Options · Select / Esc", "Options · Select / Échap",
      "Opciones · Select / Esc", "設定 · Select / Esc"),
    T("hintkeyboard", "Options · Esc", "Options · Échap", "Opciones · Esc", "設定 · Esc"),
    T("hintxbox", "Options · View", "Options · View", "Opciones · View", "設定 · View"),
    T("hintps", "Options · Share", "Options · Share", "Opciones · Share", "設定 · Share"),
    T("hintnintendo", "Options · −", "Options · −", "Opciones · −", "設定 · −"),
    T("hinttooltip", "Open options · Select / View, Esc or F10. You can also click this badge.",
      "Ouvrir les options · Select / View, Échap ou F10. Tu peux aussi cliquer sur ce badge.",
      "Abrir opciones · Select / View, Esc o F10. También puedes pulsar esta etiqueta.",
      "設定を開く · Select / View、Esc、F10。この表示のクリックでも開けます。"),
    T("hintcorner", "Reminder corner", "Coin de l’indication", "Esquina del aviso",
      "操作案内の位置"),
    T("bottomright", "Bottom right", "En bas à droite", "Abajo a la derecha", "右下"),
    T("bottomleft", "Bottom left", "En bas à gauche", "Abajo a la izquierda", "左下"),
    T("topright", "Top right", "En haut à droite", "Arriba a la derecha", "右上"),
    T("topleft", "Top left", "En haut à gauche", "Arriba a la izquierda", "左上"),
    T("hintdetail", "Reminder content", "Contenu de l’indication", "Contenido del aviso",
      "操作案内の内容"),
    T("hintcompact", "Compact · both shortcuts", "Compact · les deux commandes",
      "Compacto · ambos controles", "コンパクト · 両方の操作"),
    T("hintdevice", "Active input only", "Commande utilisée seulement", "Solo el control activo",
      "使用中の操作のみ"),
    T("hintdetailed", "Detailed · include rewind", "Détaillé · avec rewind",
      "Detallado · con rebobinado", "詳細 · 巻き戻しも表示"),
    T("hintscale", "Reminder size", "Taille de l’indication", "Tamaño del aviso",
      "操作案内の大きさ"),
    T("hintopacity", "Overlay background opacity", "Opacité du fond des indications",
      "Opacidad del fondo de los avisos", "操作案内の背景の不透明度"),
    T("hintmargin", "Overlay edge spacing", "Marge des indications", "Margen de los avisos",
      "操作案内の画面端からの余白"),
    T("hintseconds", "Hide reminder after · 0 = always visible",
      "Masquer l’indication après · 0 = toujours visible",
      "Ocultar aviso después · 0 = siempre visible", "操作案内を隠す秒数 · 0 = 常に表示"),
    T("noticeseconds", "Notification duration · 0 = until dismissed",
      "Durée des notifications · 0 = jusqu’à fermeture",
      "Duración de las notificaciones · 0 = hasta cerrarlas", "通知の表示秒数 · 0 = 閉じるまで"),
    T("rewindindicator", "Show status while rewinding", "Afficher l’état pendant le rewind",
      "Mostrar estado al rebobinar", "巻き戻し中の状態を表示"),
    T("smoothmap", "Smooth reduced maps", "Lisser les cartes réduites", "Suavizar mapas reducidos",
      "縮小マップを滑らかに表示"),
    T("settingguide", "Setting guide · Y / F1", "Guide du réglage · Y / F1",
      "Guía del ajuste · Y / F1", "設定ガイド・Y / F1"),
    T("guideclose", "Close guide · B / Esc", "Fermer le guide · B / Échap", "Cerrar guía · B / Esc",
      "ガイドを閉じる・B / Esc"),
    T("guidedrawing", "Illustration · not game footage", "Schéma · pas une capture du jeu",
      "Ilustración · no es una captura", "模式図・ゲーム画面ではありません"),
    T("guidehint",
      "Focus any setting for its explanation. Press the top face button (Y / Triangle) or F1 to "
      "open this guide; B / Esc closes it.",
      "Sélectionne un réglage pour son explication. Le bouton du haut (Y / Triangle) ou F1 ouvre "
      "le guide ; B / Échap le ferme.",
      "Selecciona un ajuste para ver su explicación. El botón superior (Y / Triángulo) o F1 abre "
      "la guía; B / Esc la cierra.",
      "項目を選ぶと説明が変わります。上ボタン（Y / △）かF1でガイドを開き、B / Escで閉じます。"),
    T("zoomout", "Zoom-out divisor", "Diviseur de dézoom", "Divisor de reducción", "縮小除数"),
    T("pixelreset", "Exact pixel 1:1", "Pixels exacts 1:1", "Píxel exacto 1:1", "等倍ピクセル表示"),
    T("effectivescale", "Effective pixel scale", "Échelle réelle des pixels",
      "Escala real de píxeles", "実際のピクセル倍率"),
    T("panoramaheight", "Panorama world height", "Hauteur du monde en panorama",
      "Altura del mundo en panorama", "パノラマの表示高さ"),
    T("mapextension", "Full Spinball / 3D Blast terrain", "Terrain complet Spinball / 3D Blast",
      "Terreno completo de Spinball / 3D Blast", "Spinball / 3D Blastの全地形"),
    T("atlascapability",
      "Full cartridge terrain is available. Distant scenery is cached; live actors and moving "
      "terrain follow the original active area.",
      "Le terrain complet de la cartouche est disponible. Le décor éloigné est en cache ; les "
      "acteurs et éléments mobiles suivent la zone active native.",
      "El terreno completo del cartucho está disponible. El paisaje lejano se almacena en caché; "
      "actores y terreno móvil siguen el área activa original.",
      "全地形を表示できます。遠景はキャッシュし、動くキャラクターと地形は本来の活動範囲に従います"
      "。"),
    T("platformcapability",
      "Expanded platform terrain is available. Streamed art may need the terrain diagram, "
      "especially in Sonic 3.",
      "Le terrain de plateforme étendu est disponible. Les graphismes progressifs peuvent "
      "nécessiter le schéma de terrain, surtout dans Sonic 3.",
      "El terreno ampliado está disponible. Los gráficos por zonas pueden necesitar el esquema, "
      "sobre todo en Sonic 3.",
      "広域地形表示に対応しています。特にソニック3の逐次読込画像では地形図が必要な場合があります"
      "。"),
    T("nativecapability",
      "This scene uses its original complete screen. Puzzle boards, special stages and unsupported "
      "scenes do not expose an extended terrain map.",
      "Cette scène utilise son écran original complet. Les puzzles, stages spéciaux et scènes non "
      "compatibles n’exposent pas de terrain étendu.",
      "Esta escena usa su pantalla original completa. Puzles, fases especiales y escenas no "
      "compatibles no exponen un mapa ampliado.",
      "この場面は本来の画面を表示します。パズル・特別ステージ・非対応場面には広域地形図がありません"
      "。"),
    T("bosspreview", "Preview bosses", "Aperçu des boss", "Vista previa de jefes",
      "ボスの出現位置を表示"),
    T("scriptpreview", "Preview scripted spawns", "Aperçu des apparitions scriptées",
      "Vista previa de apariciones programadas", "イベントによる出現を表示"),
    T("spawntint", "Tint boss and scripted ghosts", "Teinter les boss et les apparitions scriptées",
      "Colorear jefes y apariciones programadas", "ボスとイベントの色分け"),
    T("spawnlabels", "Label bosses and scripted origins",
      "Étiquettes des boss et des origines scriptées", "Etiquetas de jefes y orígenes programados",
      "ボスと出現元のラベル"),
    T("fitspawns", "Include distant spawns in stage overview",
      "Inclure les apparitions éloignées dans le cadrage",
      "Incluir apariciones lejanas en la vista completa", "全体表示に遠くの出現位置を含める"),
    T("bosslabel", "BOSS", "BOSS", "JEFE", "ボス"),
    T("scriptlabel", "SCRIPT", "SCRIPT", "EVENTO", "イベント"),
    T("spawnorigin", "ORIGIN · position varies", "ORIGINE · position variable",
      "ORIGEN · posición variable", "出現元・位置は変動"),
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
      "Amber: enemies. Red: bosses. Purple: scripted objects and projectiles. Fixed points use "
      "the cartridge's placements and supported event programs. ORIGIN marks a source whose "
      "attack position varies; it is not a trajectory prediction. Parent-relative points follow "
      "their live source. Ghosts disappear when their native sprite is visible. Remembered "
      "defeated objects are hidden. Markers cover unavailable art. Platform stages only.",
      "Ambre : ennemis. Rouge : boss. Violet : objets et tirs scriptés. Les points fixes viennent "
      "des placements et des événements reconnus de la cartouche. ORIGINE indique une source "
      "dont la position d’attaque varie, sans prédire la trajectoire. Les points relatifs suivent "
      "leur source active. Les aperçus disparaissent quand le sprite natif est visible. Les objets "
      "détruits sont masqués si le jeu les mémorise. Repères si l’image manque. Plateforme "
      "uniquement.",
      "Ámbar: enemigos. Rojo: jefes. Violeta: objetos y proyectiles programados. Los puntos fijos "
      "proceden de posiciones y eventos reconocidos del cartucho. ORIGEN indica una fuente cuya "
      "posición de ataque varía; no predice trayectorias. Los puntos relativos siguen su fuente. "
      "Las vistas previas se ocultan al verse el sprite original o si el juego recuerda su "
      "derrota. "
      "Los marcadores cubren imágenes no disponibles. Solo fases de plataformas.",
      "黄色は敵、赤はボス、紫はイベントのオブジェクトと弾です。固定位置はカートリッジの配置と対応す"
      "る"
      "イベントから読み取ります。出現元は攻撃の位置が変動する場所で、弾道の予測ではありません。相対"
      "位置は"
      "動く出現元を追います。元の画像が見える場合や倒した記録がある場合は非表示です。画像がない場合"
      "は"
      "マーカーを使います。通常ステージで使えます。"),
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
      "The supported current map fits on screen with a player marker and optional live close-up. "
      "Spinball and 3D Blast include complete cartridge terrain. Puzzle boards and special stages "
      "keep their native screen.",
      "La carte actuelle compatible tient à l’écran avec un repère du joueur et une vue native "
      "optionnelle. Spinball et 3D Blast incluent leur terrain complet. Puzzles et stages spéciaux "
      "gardent leur écran natif.",
      "El mapa actual compatible cabe en pantalla con marcador y vista nativa opcional. Spinball y "
      "3D Blast incluyen su terreno completo. Puzles y fases especiales conservan su pantalla "
      "original.",
      "対応マップ全体とプレイヤーマーカー、任意の拡大画面を表示します。Spinballと3D "
      "Blastは全地形に対応し、パズルと特別ステージは本来の画面を使います。"),
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
      "the shoulder to play. Select/View always opens options. Hold L2/LT or Backspace to rewind "
      "at 1x speed with reversed sound (default binding).",
      "En jeu, maintiens le bouton d’épaule droit : haut/bas = zoom; bouton du haut = niveau "
      "entier; bouton gauche = figer/suivre; bouton droit = centrer; Start = capture. Relâche "
      "l’épaule pour jouer. Select/View ouvre les options. Maintiens L2/LT ou Backspace pour "
      "rembobiner à vitesse normale avec le son inversé (bouton par défaut).",
      "En juego, mantén el hombro derecho: arriba/abajo = zoom; botón superior = nivel entero; "
      "izquierdo = fijar/seguir; derecho = centrar; Start = captura. Suelta el hombro para jugar. "
      "Select/View abre opciones. Mantén L2/LT o Backspace para rebobinar a velocidad normal "
      "con sonido invertido (botón predeterminado).",
      "ゲーム中は右ショルダーを押しながら操作: "
      "上下でズーム、上ボタンで全体表示、左ボタンで固定・追従、右ボタンでプレイヤーへ、Start "
      "で撮影。ショルダーを離すと通常操作。Select/View で設定。既定では L2/LT または Backspace "
      "を押し続けると通常速度で映像と音声を巻き戻します。"),
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
    T("panorama", "Panorama / adjustable height", "Panorama / hauteur réglable",
      "Panorámica / altura ajustable", "パノラマ・高さ調整"),
    T("overview", "Whole-level overview", "Vue du niveau entier", "Vista de todo el nivel",
      "ステージ全体"),
    T("classic", "Original game framing", "Cadre original du jeu", "Encuadre original del juego",
      "元のゲーム画面"),
    T("zoom", "Integer zoom", "Zoom entier", "Zoom entero", "整数ズーム"),
    T("pixelhelp",
      "Magnification 1× and divisor 1× give exact physical 1:1. Increase the divisor to zoom out "
      "below 1:1; + / − also crosses that boundary.",
      "Agrandissement 1× et diviseur 1× donnent le 1:1 physique exact. Augmente le diviseur pour "
      "dézoomer davantage ; + / − franchit aussi cette limite.",
      "Aumento 1× y divisor 1× dan 1:1 físico exacto. Aumenta el divisor para alejarte más; + / − "
      "también cruza ese límite.",
      "拡大率1倍・除数1倍で物理画素の等倍表示です。除数を増やすとさらに広く表示できます。+ / "
      "−でも等倍を越えて縮小できます。"),
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
      "Alt + Enter\nFree camera: WASD / right stick · Rewind 1x: hold L2/LT or Backspace",
      "Bouger : flèches · Saut / action : Z, X, C · Start : Entrée\nManette : croix / stick gauche "
      "· A, X, B · Start\nMenu : Select / View ou Échap / F10 · Valider : A / Entrée · Retour : B "
      "/ Échap\nZoom : + / − ou Ctrl + molette · Figer : F · Suivre : R\nSauver : Maj + F1…F9 · "
      "Charger : F1…F9 · Plein écran : Alt + Entrée\nCaméra libre : WASD / stick droit · Retour "
      "arrière 1x : maintenir L2/LT ou Backspace",
      "Mover: flechas · Salto / acción: Z, X, C · Start: Intro\nMando: cruceta / stick izquierdo · "
      "A, X, B · Start\nMenú: Select / View o Esc / F10 · Confirmar: A / Intro · Volver: B / "
      "Esc\nZoom: + / − o Ctrl + rueda · Fijar: F · Seguir: R\nGuardar: Mayús + F1…F9 · Cargar: "
      "F1…F9 · Pantalla completa: Alt + Intro\nCámara libre: WASD / stick derecho · Rebobinar: "
      "mantener L2/LT o Retroceso (1x)",
      "移動: 矢印キー ・ ジャンプ / アクション: Z, X, C ・ Start: Enter\nゲームパッド: 十字キー / "
      "左スティック ・ A, X, B ・ Start\nメニュー: Select / View または Esc / F10 ・ 決定: A / "
      "Enter ・ 戻る: B / Esc\nズーム: + / − または Ctrl + ホイール ・ 固定: F ・ 追従: R\n保存: "
      "Shift + F1…F9 ・ 読込: F1…F9 ・ 全画面: Alt + Enter\n自由カメラ: WASD / 右スティック ・ "
      "巻き戻し 1x: L2/LT または Backspace を押し続ける"),
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
    T("rewindbutton", "Rewind gamepad button", "Bouton de rewind", "Botón para rebobinar",
      "巻き戻しボタン"),
    T("rewindaudio", "Reverse sound during rewind", "Son inversé pendant le rewind",
      "Invertir el sonido al rebobinar", "巻き戻し中の音声を逆再生"),
    T("rewindmemory", "Rewind memory budget", "Mémoire du rewind", "Memoria del rebobinado",
      "巻き戻しのメモリ上限"),
    T("rewindhint", "Rewind: %s / Backspace · %.1f s", "Rewind : %s / Backspace · %.1f s",
      "Rebobinar: %s / Backspace · %.1f s", "巻き戻し: %s / Backspace · %.1f 秒"),
    T("rewinding", "Rewind 1x · %s · %.1f s", "Rewind 1x · %s · %.1f s",
      "Rebobinado 1x · %s · %.1f s", "巻き戻し 1x · %s · %.1f 秒"),
    T("rewindhelp",
      "Hold L2/LT (default) or Backspace: picture and stereo sound run backwards at normal "
      "speed. Release to continue from there. History starts when you play and is cleared by "
      "loading, restarting or switching games. Duration depends on the game and memory budget; "
      "Lightweight caps it at 24 MB. Nothing is recorded to disk.",
      "Maintiens L2/LT (par défaut) ou Backspace : l’image et le son stéréo reculent à vitesse "
      "normale. Relâche pour repartir de là. L’historique se crée en jouant et s’efface au "
      "chargement, au redémarrage ou au changement de jeu. Sa durée dépend du jeu et de la "
      "mémoire ; le mode léger limite à 24 Mo. Rien n’est enregistré sur disque.",
      "Mantén L2/LT (predeterminado) o Backspace: imagen y sonido estéreo retroceden a velocidad "
      "normal. Suelta para continuar desde ahí. El historial se crea al jugar y se borra al "
      "cargar, reiniciar o cambiar de juego. La duración depende del juego y de la memoria; "
      "Ligero limita a 24 MB. No se graba nada en el disco.",
      "L2/LT (既定) または Backspace を押し続けると映像とステレオ音声を通常速度で逆再生します。"
      "離すとその時点から再開します。履歴はプレイ中に作成され、ロード、再起動、ゲームの変更で消えま"
      "す。"
      "保持時間はゲームとメモリ上限によって変わります。軽量モードは 24 MB までです。"
      "ディスクには記録しません。"),
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
    T("help", "Show compact options reminder", "Afficher l’indication compacte des options",
      "Mostrar aviso compacto de opciones", "コンパクトな操作案内を表示"),
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
std::string translationGlyphs() {
    std::string result;
    for (const auto &entry : texts)
        for (auto text : entry.text)
            result += text;
    return result;
}
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
