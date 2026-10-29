#include "setting_help.h"
#include <cstring>
#include <initializer_list>
namespace scu {
#define H(key, picture, en, fr, es, ja)                                                            \
    {                                                                                              \
        key, HelpPicture::picture, {                                                               \
            en, fr, es, ja                                                                         \
        }                                                                                          \
    }
static const SettingHelp entries[] = {
    H("desktoppresentation", Crt,
      "Choose the ordinary canvas, a whole-board pinball cabinet frame, or a CRT with the expanded "
      "level behind it. Desktop frames use portable 2D geometry on SDL renderers; the Quest "
      "edition adds stereo 3D furniture and tube shaders. Frame size changes presentation, not "
      "game speed or controls.",
      "Choisis le canevas habituel, un cadre de flipper avec plateau entier ou un CRT avec le "
      "niveau derrière. Sur PC, les cadres utilisent une géométrie 2D portable avec SDL ; sur "
      "Quest, le mobilier et le tube sont rendus en stéréo 3D. La taille du cadre ne change ni "
      "vitesse ni commandes.",
      "Elige lienzo normal, marco de pinball completo o CRT con nivel detrás. Los marcos de "
      "escritorio usan geometría 2D portable con SDL; Quest añade muebles y tubo en 3D estéreo. El "
      "tamaño no cambia velocidad ni controles.",
      "通常のキャンバス、全盤面のピンボール筐体フレーム、周囲にステージを展開するブラウン管を選びま"
      "す。デスクトップはSDLの移植可能な2D描画、Questは立体3D筐体と管面シェーダーを使用します。サイ"
      "ズはゲーム速度や操作を変えません。"),
    H("questscale", Resize,
      "Change physical size from 5% to 20,000% without increasing texture memory. Both grips "
      "resize even with placement locked. Spread your hands, regrip to continue, or use "
      "right-stick up/down while gripping. Half, double and reset are available here. The nearby "
      "menu keeps its own size.",
      "Change la taille physique de 5 à 20 000 % sans augmenter la mémoire des textures. Les deux "
      "prises redimensionnent même en placement verrouillé. Écarte les mains, reprends les prises "
      "ou utilise le stick droit haut/bas. Demi-taille, double et remise à zéro sont disponibles. "
      "Le menu proche garde sa taille.",
      "Cambia el tamaño físico del 5 al 20 000 % sin aumentar memoria de texturas. Ambos agarres "
      "redimensionan incluso con posición bloqueada. Separa las manos, vuelve a agarrar o usa la "
      "palanca derecha arriba/abajo. Hay mitad, doble y reinicio. El menú cercano mantiene su "
      "tamaño.",
      "テクスチャメモリを増やさず実寸を5〜20,000%"
      "に変更します。位置固定中でも両グリップで拡縮。手を広げ、握り直すか右スティック上下で続けます"
      "。半分・2倍・リセットも使用可能。近くのメニューの大きさは変わりません。"),
    H("questspinballauto", Pinball,
      "When Sonic Spinball is loaded, select its complete-board 3D cabinet automatically. Other "
      "games retain the presentation chosen above. Disable this to use murals, the CRT or any "
      "other mode for Spinball. Cartridge title and unsupported scenes retain a native fallback.",
      "Au chargement de Sonic Spinball, sélectionne automatiquement le meuble 3D et le plateau "
      "entier. Les autres jeux gardent le mode choisi au-dessus. Désactive pour utiliser le mural, "
      "le CRT ou un autre mode avec Spinball. Le titre et les scènes non compatibles gardent une "
      "vue native.",
      "Al cargar Sonic Spinball, selecciona automáticamente su mueble 3D completo. Los otros "
      "juegos conservan el modo elegido arriba. Desactívalo para usar mural, CRT u otro modo con "
      "Spinball. Títulos y escenas no compatibles conservan una vista nativa.",
      "Sonic "
      "Spinballを読み込むと全盤面の3D筐体を自動選択。他のゲームは選択中の表示を維持します。Spinball"
      "で壁画やブラウン管を使うには無効にします。タイトルや未対応の場面には元の画面を使用します。"),
    H("questcabinetwidth", Pinball,
      "Width sets the playfield in centimetres; depth follows the complete board's true aspect. "
      "Front height measures from the floor, and slope raises the rear. Global physical scale "
      "multiplies the whole cabinet, including its legs and backglass. The original 2D playfield "
      "and physics remain authentic.",
      "La largeur règle le plateau en centimètres ; sa profondeur suit les proportions du plateau "
      "entier. La hauteur avant se mesure depuis le sol ; l’inclinaison relève l’arrière. "
      "L’échelle globale multiplie le meuble entier, pieds et fronton compris. Le plateau 2D et la "
      "physique restent ceux du jeu.",
      "La anchura define el tablero en centímetros; la profundidad conserva su proporción. La "
      "altura frontal se mide desde el suelo y la inclinación eleva la parte trasera. La escala "
      "global multiplica todo el mueble, patas y panel incluidos. Se mantienen tablero 2D y física "
      "originales.",
      "幅をcmで設定し、奥行きは全盤面の比率に従います。前面高さは床から測り、傾斜は奥を持ち上げます"
      "。全体スケールは脚とバックボードも拡縮。盤面の2D画像と物理動作は元のゲームを保持します。"),
    H("questglass", Pinball,
      "A light, view-dependent glass reflection above the board. Set zero for maximum visibility "
      "and lower blending cost. Reflections do not hide the ball with an opaque overlay.",
      "Reflet léger du verre selon le point de vue, au-dessus du plateau. Zéro maximise la "
      "visibilité et réduit le mélange graphique. Aucun reflet opaque ne cache la bille.",
      "Reflejo suave del cristal según el punto de vista. Cero ofrece máxima visibilidad y reduce "
      "el coste de mezcla. No se oculta la bola con reflejos opacos.",
      "視点に応じた軽いガラス反射です。0で視認性を最大にし、合成負荷を減らします。不透明な反射でボ"
      "ールを隠しません。"),
    H("questtvwidth", Crt,
      "Set the native CRT picture width in centimetres. Its height keeps the cartridge aspect "
      "ratio. The cabinet and stand follow it; global scale multiplies the complete object. "
      "Surrounding terrain aligns to the original camera at this same pixel scale.",
      "Règle la largeur de l’image CRT en centimètres. Sa hauteur respecte les proportions de la "
      "cartouche. Le meuble et le pied suivent ; l’échelle globale multiplie l’objet entier. Le "
      "terrain autour s’aligne sur la caméra native à cette même échelle de pixels.",
      "Define la anchura de la imagen CRT en centímetros. La altura conserva la proporción del "
      "cartucho. Mueble y soporte se adaptan; la escala global multiplica todo. El terreno "
      "alrededor se alinea con la cámara original a la misma escala de píxel.",
      "元の映像幅をcmで設定します。高さはカートリッジの比率を保持。筐体と台座も追従し、全体スケール"
      "で全体を拡縮します。周囲の地形は同じピクセル密度で元のカメラに合わせます。"),
    H("questcrt", Crt,
      "Clean, living-room and arcade presets increase scanline contrast. Off gives an unfiltered "
      "picture in the 3D TV. Shader scanlines and the RGB mask fade when too small to resolve, "
      "reducing shimmer. Strength blends the effect; curvature changes the actual tube mesh.",
      "Les profils moniteur, salon et arcade renforcent progressivement les lignes de balayage. "
      "Désactivé conserve une image brute dans le téléviseur 3D. Les lignes et le masque RVB "
      "s’effacent quand ils sont trop fins, pour réduire le scintillement. L’intensité dose "
      "l’effet ; la courbure modifie le maillage du tube.",
      "Monitor, doméstico y arcade aumentan el contraste de líneas. Desactivado conserva la imagen "
      "sin filtro dentro del TV 3D. Líneas y máscara RGB se atenúan al ser demasiado pequeñas para "
      "evitar parpadeos. La intensidad mezcla el efecto; la curvatura modifica la malla del tubo.",
      "クリア・家庭用・アーケードの順に走査線を強めます。オフでは3Dテレビ内に未処理の映像を表示。細"
      "かすぎる走査線とRGBマスクを弱め、ちらつきを抑えます。強度で効果を調整し、湾曲は画面メッシュ"
      "を変更します。"),
    H("questsurround", Crt,
      "Place expanded cartridge terrain around and behind the native CRT viewport, aligned with "
      "its camera. The TV remains the real game view. Brightness dims the surrounding world for "
      "focus. Native-only scenes omit the extension. This is the supported current stage, not "
      "invented or downloaded scenery.",
      "Place le terrain étendu autour et derrière la vue native du CRT, aligné sur sa caméra. Le "
      "téléviseur reste la vraie vue du jeu. La luminosité atténue le monde autour. Les scènes "
      "natives n’ont pas d’extension. Il s’agit du niveau compatible actuel, sans décor inventé ou "
      "téléchargé.",
      "Coloca terreno ampliado alrededor y detrás del CRT, alineado con su cámara. El TV conserva "
      "la vista real. El brillo permite atenuar el entorno. Las escenas solo nativas omiten la "
      "extensión. Se muestra el nivel actual compatible, sin decorado inventado o descargado.",
      "拡張したカートリッジの地形を元のカメラに合わせ、ブラウン管の周囲と背後に表示します。テレビ内"
      "は実際のゲーム画面です。周囲の明るさを下げて集中できます。未対応場面では拡張せず、現在の対応"
      "ステージのみ使用します。"),
    H("questcurve", Wall,
      "Wrap the entire supported stage around a curved mesh from 10 to 220 degrees. Physical "
      "height and scale determine size; the menu stays flat and nearby. A very wide level can "
      "surround you: resize or reposition it to suit your seated view.",
      "Enroule le niveau entier sur un maillage incurvé de 10 à 220 degrés. La hauteur et "
      "l’échelle déterminent la taille ; le menu reste plat et proche. Un niveau très large peut "
      "t’entourer : redimensionne ou replace-le pour ton point de vue assis.",
      "Envuelve el nivel completo en una malla curva de 10 a 220 grados. Altura y escala definen "
      "el tamaño; el menú sigue plano y cercano. Un nivel muy ancho puede rodearte: ajusta tamaño "
      "y posición para jugar sentado.",
      "対応ステージ全体を10〜220度の湾曲メッシュに展開。高さとスケールが大きさを決め、メニューは平"
      "面で近くに残ります。横長のステージは周囲を囲めます。着席位置に合わせて拡縮・配置してください"
      "。"),
    H("questenvironment", Quest,
      "Choose a dark void or a quiet procedural studio floor in stereo modes. Passthrough replaces "
      "the studio with your actual room. No camera frames are read or recorded by the application.",
      "Choisis un fond sombre ou un sol de studio procédural discret dans les modes stéréo. Le "
      "passthrough remplace le studio par ta pièce réelle. L’application ne lit ni n’enregistre "
      "les images des caméras.",
      "Elige vacío oscuro o suelo de estudio procedural en modos estéreo. Passthrough sustituye el "
      "estudio por tu habitación. La aplicación no lee ni graba imágenes de cámara.",
      "立体モードで暗い空間か静かな手続き型スタジオの床を選びます。パススルーはスタジオを実際の部屋"
      "に置き換えます。アプリはカメラ映像を取得・録画しません。"),
    H("questmode", Wall,
      "Choose a normal spatial screen, scanned wall, full-stage mural, floor atlas, 3D pinball cabinet, 3D CRT with surrounding terrain, or curved stage gallery. The complete current stage keeps fixed framing in map modes. Unsupported scenes keep their native viewport. Furniture modes always keep options nearby.",
      "Choisis écran spatial, mur scanné, mural complet, atlas au sol, flipper 3D, CRT 3D avec terrain autour ou galerie incurvée. Le niveau actuel entier reste fixe dans les modes carte. Les scènes non compatibles gardent leur vue native. Le mobilier garde toujours les options à proximité.",
      "Elige pantalla espacial, pared capturada, mural completo, atlas de suelo, pinball 3D, CRT 3D con terreno alrededor o galería curva. El nivel completo mantiene encuadre fijo en modos mapa. Las escenas no compatibles usan su vista nativa. Los muebles mantienen las opciones cerca.",
      "空間スクリーン、スキャンした壁、全ステージ壁画、床の地図、3Dピンボール筐体、周囲に地形を表示する3Dブラウン管、湾曲ギャラリーを選びます。地図モードは全ステージを固定表示。未対応場面は元の画面を使い、家具モードの設定は常に近くに表示します。"),
    H("questwallheight", Wall,
      "Default 200 cm: the mural rises two metres from the floor. Width follows the stage aspect "
      "ratio. Wall-fit mode caps height to the available wall. On a floor atlas this sets its "
      "depth. This changes physical size, not game speed.",
      "Par défaut 200 cm : le mural monte de deux mètres depuis le sol. La largeur suit les "
      "proportions du niveau. Ajuster au mur limite la hauteur à celle disponible. Pour l’atlas, "
      "cela règle sa profondeur. La vitesse du jeu reste normale.",
      "Por defecto 200 cm: el mural sube dos metros desde el suelo. El ancho sigue el nivel. "
      "Ajustar a pared limita la altura disponible. En el atlas controla la profundidad. La "
      "velocidad del juego no cambia.",
      "初期値200cmでは床から高さ2mに表示します。幅はステージの比率に従います。壁合わせでは壁の高さ"
      "までに制限します。床の地図では奥行きを設定します。ゲーム速度は変わりません。"),
    H("questwallgap", Wall,
      "Raise the mural's lower edge above the floor. Zero starts at floor level. The floor atlas "
      "uses this as elevation. Wall-fit mode limits the gap to keep the screen within its wall.",
      "Monte le bord inférieur du mural au-dessus du sol. Zéro commence au niveau du sol. Pour "
      "l’atlas, règle sa hauteur au-dessus du sol. Ajuster au mur limite cet écart pour rester "
      "dans le mur.",
      "Eleva el borde inferior sobre el suelo. Cero empieza en el suelo. En el atlas controla la "
      "elevación. Ajustar a pared limita el espacio para no salir de sus bordes.",
      "壁画の下端と床の間隔です。0では床から始まります。床の地図では床からの高さです。壁合わせでは"
      "壁からはみ出さないよう制限します。"),
    H("questwalldistance", Wall,
      "Choose the distance used by Place virtual wall in front of me, then activate that button. "
      "Changing this preference alone does not move a locked mural.",
      "Choisis la distance utilisée par Placer le mur virtuel devant moi, puis active ce bouton. "
      "Ce réglage seul ne déplace pas un mural verrouillé.",
      "Elige la distancia de Colocar pared virtual delante y activa ese botón. Este ajuste por sí "
      "solo no mueve un mural bloqueado.",
      "正面に仮想の壁を配置する際の距離です。設定後に配置ボタンを押してください。この設定だけでは固"
      "定した壁画は動きません。"),
    H("questautowall", Wall,
      "Load the saved room model when entering a wall mode. Quest asks for spatial-data "
      "permission. The app prefers a nearby wall in your viewing direction and keeps that choice. "
      "Without a usable model, manual placement remains available, including outdoors.",
      "Charge le modèle de pièce en mode mural. Le Quest demande l’accès aux données spatiales. "
      "L’application préfère un mur proche dans ta direction et conserve ce choix. Sans modèle "
      "utilisable, le placement manuel reste disponible, y compris dehors.",
      "Carga el modelo guardado al entrar en modo pared. Quest pide permiso espacial. Se prefiere "
      "una pared cercana en tu dirección y se mantiene esa elección. Sin modelo puedes colocarla "
      "manualmente, también fuera.",
      "壁モードで保存済みの部屋を読み込みます。Questが空間データの許可を求めます。正面付近の近い壁"
      "を選び、その選択を維持します。モデルがない場合は屋外でも手動配置を使えます。"),
    H("questwalllock", Wall,
      "Keep the wall or furniture in place while looking around or recentering. Both grips always "
      "resize it. Unlock only when you want both grips to translate it too. Full-stage framing "
      "remains fixed.",
      "Garde le mur ou le meuble en place pendant les mouvements de tête et le recentrage. Les "
      "deux prises redimensionnent toujours. Déverrouille pour qu’elles déplacent aussi l’objet. "
      "Le cadrage du niveau reste fixe.",
      "Mantén pared o mueble fijo al mirar y recentrar. Ambos agarres siempre redimensionan. "
      "Desbloquea solo para que también muevan el objeto. El encuadre completo permanece fijo.",
      "頭を動かしたり再センタリングしても壁や筐体の位置を保ちます。両グリップの拡縮は常に可能。移動"
      "も行う場合だけ解除します。全ステージの範囲は固定したままです。"),
    H("questpauselost", Wall,
      "Pause play if the attached wall anchor or head pose becomes unavailable. The mural is "
      "hidden instead of attaching to another wall. With head tracking restored, Y and the "
      "companion menu allow restoration or manual placement.",
      "Met le jeu en pause si l’ancrage ou la tête ne peut plus être localisé. Le mural est masqué "
      "et ne passe pas sur un autre mur. Quand la tête est suivie, Y et le menu de proximité "
      "permettent de restaurer ou replacer le mur.",
      "Pausa si no se puede localizar el anclaje o la cabeza. El mural se oculta sin pasar a otra "
      "pared. Al recuperar la cabeza, Y y el menú permiten restaurar o colocar de nuevo.",
      "アンカーや頭の位置が取得できない間は一時停止します。別の壁に移さず、壁画を非表示にします。頭"
      "の追跡が戻ったらYのメニューから復元または再配置できます。"),
    H("questcompanion", Wall,
      "Open Y options on a separate nearby screen while the mural stays on the wall. Turning this "
      "off places options on the mural. In options, right-stick click always recovers a nearby "
      "menu without moving the wall.",
      "Ouvre les options Y sur un écran séparé près de toi, sans déplacer le mural. Désactivé, les "
      "options restent sur le mural. Dans les options, le clic du stick droit ramène toujours un "
      "menu proche sans déplacer le mur.",
      "Abre las opciones Y en una pantalla cercana sin mover el mural. Desactivado, aparecen en el "
      "mural. Dentro de opciones, pulsar la palanca derecha siempre recupera un menú cercano sin "
      "mover la pared.",
      "Yの設定を近くの別画面に表示し、壁画を動かしません。無効にすると設定も壁画上に表示します。設"
      "定中は右スティック押下でいつでもメニューを近くに戻せます。"),
    H("questmenudistance", Wall,
      "Set the distance of the companion options panel. Its physical size scales with distance to "
      "keep a readable angular size. Right-stick click positions it in front of you again.",
      "Règle la distance du menu proche. Sa taille physique suit la distance pour conserver une "
      "taille angulaire lisible. Le clic du stick droit le replace devant toi.",
      "Ajusta la distancia del menú. Su tamaño físico sigue la distancia para conservar un tamaño "
      "angular legible. La palanca derecha lo devuelve delante.",
      "操作メニューまでの距離です。距離に合わせて物理サイズを変え、見かけの大きさを保ちます。右ステ"
      "ィック押下で正面に戻します。"),
    H("questwalldetail", Performance,
      "Fast, Balanced and Sharp bound the mural's texture and map-sampling budgets. Larger "
      "textures give more detail up close but cost memory and rendering time. Runtime texture "
      "limits still apply; a huge map may be sampled down.",
      "Rapide, Équilibré et Détaillé règlent les budgets de texture et d’échantillonnage. Plus de "
      "détail de près demande plus de mémoire et de temps de rendu. Les limites du casque "
      "s’appliquent ; une grande carte peut être réduite.",
      "Rápido, Equilibrado y Nítido limitan la textura y el muestreo. Más detalle de cerca consume "
      "memoria y tiempo. Se respetan los límites del visor; los mapas grandes pueden reducirse.",
      "軽量・バランス・高精細で壁画のテクスチャと地図の描画量を設定します。近くでの細かさとメモリ・"
      "描画時間の調整です。大きな地図は本体の上限に合わせて縮小される場合があります。"),
    H("questadaptive", Performance,
      "Reduce mural rendering resolution after sustained CPU budget pressure, then restore detail "
      "conservatively. Headset refresh preference, physical size, map bounds and native game speed "
      "stay unchanged. Disable to use a fixed detail budget.",
      "Réduit la résolution du mural après une charge CPU durable, puis remonte prudemment le "
      "détail. Fréquence demandée, taille physique, cadrage et vitesse du jeu restent identiques. "
      "Désactive pour un budget fixe.",
      "Reduce la resolución tras carga de CPU sostenida y recupera el detalle gradualmente. No "
      "cambia la frecuencia solicitada, el tamaño físico, el mapa ni la velocidad del juego. "
      "Desactívalo para un presupuesto fijo.",
      "CPUの処理時間が継続して不足すると解像度を下げ、余裕が続けば慎重に戻します。要求する更新頻度"
      "、物理サイズ、地図の範囲、ゲーム速度は変えません。固定品質にするには無効にします。"),
    H("questplacewall", Wall,
      "Create a manual virtual wall in front of your current horizontal viewing direction, at the "
      "chosen distance and floor height. It does not detect a real surface. It stays still after "
      "placement and can be saved as a local anchor.",
      "Place un plan virtuel dans ta direction horizontale, à la distance choisie et au niveau du "
      "sol. Ce bouton ne détecte pas une surface réelle. Le mur reste fixe et peut être enregistré "
      "comme ancrage local.",
      "Coloca un plano virtual en tu dirección horizontal, a la distancia y suelo elegidos. No "
      "detecta una superficie real. Queda fijo y puede guardarse como anclaje local.",
      "現在の水平方向の正面に、選んだ距離と床の高さで仮想の壁を置きます。実際の面を検出する操作では"
      "ありません。配置後は固定され、本体のアンカーとして保存できます。"),
    H("questfindwall", Wall,
      "Reload nearby saved room surfaces and select a wall, preferring your viewing direction. "
      "Spatial permission and a captured room are required. Your head direction is used only for "
      "this selection, not to make the wall follow you.",
      "Recharge les surfaces de la pièce et choisit un mur, de préférence dans ta direction. Une "
      "pièce capturée et l’autorisation spatiale sont nécessaires. La direction sert au choix ; le "
      "mur ne suit pas ta tête.",
      "Recarga las superficies y elige una pared, preferiblemente en tu dirección. Requiere "
      "habitación capturada y permiso espacial. La dirección sirve para elegir; la pared no sigue "
      "tu cabeza.",
      "保存した部屋の面を読み直し、視線の方向に近い壁を選びます。部屋のスキャンと空間データの許可が"
      "必要です。頭の向きは選択時だけ使い、壁を追従させません。"),
    H("questnextwall", Wall,
      "Select the next usable wall in the loaded nearby room surfaces. The new choice remains "
      "fixed until you select another wall or place it manually.",
      "Choisit le mur suivant parmi les surfaces proches et localisables. Ce choix reste fixe "
      "jusqu’au prochain changement ou placement manuel.",
      "Elige la siguiente pared cercana localizable. Esa elección permanece fija hasta cambiarla o "
      "colocarla manualmente.",
      "読み込んだ部屋の近くにある、追跡可能な次の壁を選びます。次に選択または手動配置するまで固定し"
      "ます。"),
    H("questscanroom", Wall,
      "Open Meta's Space Setup to capture or update the current room. Finish the system flow and "
      "return to the app. The app then reloads surfaces. This needs spatial permission and is not "
      "a continuous outdoor wall scanner.",
      "Ouvre Space Setup de Meta pour capturer ou actualiser la pièce. Termine la procédure "
      "système, puis reviens. L’application recharge ensuite les surfaces. Ce n’est pas un scanner "
      "continu de murs extérieurs.",
      "Abre Space Setup de Meta para capturar o actualizar la habitación. Completa el proceso y "
      "vuelve. Después se recargan las superficies. No es un escáner continuo de paredes "
      "exteriores.",
      "MetaのSpace "
      "Setupで部屋をスキャン・更新します。システムの操作を完了して戻ると面を再読込します。空間デー"
      "タの許可が必要です。屋外の壁を常時検出する機能ではありません。"),
    H("questcalibratefloor", Wall,
      "Put the left controller at the floor height you want, then activate this button with the "
      "right controller or menu navigation. Its tracked position sets the manual floor and "
      "replaces the wall in front of you. Save the placement if desired.",
      "Place la manette gauche à la hauteur du sol voulue, puis active ce bouton avec la droite ou "
      "la navigation. Cette position définit le sol manuel et replace le mur devant toi. "
      "Enregistre le placement au besoin.",
      "Pon el mando izquierdo a la altura del suelo deseada y activa este botón con el derecho o "
      "la navegación. Esa posición define el suelo manual y recoloca la pared delante. Puedes "
      "guardar la colocación.",
      "左コントローラーを床にしたい高さへ置き、右コントローラーかメニュー操作で実行します。その追跡"
      "位置を床とし、壁を正面へ再配置します。必要なら配置を保存してください。"),
    H("questsavewall", Wall,
      "Create and save an on-device spatial anchor for this placement and wall bounds. It can be "
      "located in another session in the same recognizable space. No cloud service is used. A "
      "failed save keeps the previous local record.",
      "Crée un ancrage dans le casque pour ce placement et les dimensions du mur. Il peut être "
      "retrouvé dans une autre session si l’espace est reconnu. Aucun cloud. Un échec conserve le "
      "précédent enregistrement local.",
      "Crea un anclaje en el visor para esta colocación y los límites de pared. Puede localizarse "
      "en otra sesión si se reconoce el espacio. No usa nube. Si falla, se conserva el registro "
      "anterior.",
      "配置と壁の範囲を本体の空間アンカーに保存します。同じ空間を認識できれば次のセッションでも探索"
      "できます。クラウドは使いません。失敗した場合は以前の記録を保持します。"),
    H("questrestorewall", Wall,
      "Try to locate the last saved local wall anchor. Room changes, missing tracking or runtime "
      "limitations may prevent restoration. Manual placement and room scanning remain available if "
      "it cannot be found.",
      "Essaie de localiser le dernier ancrage du mur. Une pièce modifiée, un suivi insuffisant ou "
      "le système peuvent empêcher la restauration. Le placement manuel et la capture de pièce "
      "restent disponibles.",
      "Intenta localizar el último anclaje guardado. Cambios de habitación, seguimiento o límites "
      "del sistema pueden impedirlo. Siguen disponibles la colocación manual y la captura.",
      "最後に保存した壁のアンカーを探索します。部屋の変化、追跡不足、実行環境の制約で復元できない場"
      "合があります。その場合も手動配置や部屋スキャンを利用できます。"),
    H("questrefresh", Performance,
      "Maximum available is the default: request the highest rate reported by the runtime. Choose "
      "System or an available rate instead. The active rate is displayed below. The runtime can "
      "limit the request; native game speed remains normal.",
      "Maximum disponible est le défaut : demande la plus haute fréquence annoncée par le casque. "
      "Tu peux choisir Système ou une fréquence proposée. La fréquence active est indiquée. Le "
      "casque peut limiter la demande ; le jeu garde sa vitesse.",
      "Máxima disponible pide la mayor frecuencia indicada por el visor. Puedes elegir Sistema u "
      "otra disponible. Debajo se muestra la frecuencia activa. El visor puede limitarla; el juego "
      "mantiene su velocidad normal.",
      "初期設定は利用可能な最大値です。実行環境が示す最大の更新頻度を要求します。システム任せや表示"
      "された値にも変更できます。実際の値は下に表示します。要求が制限されてもゲーム速度は通常のまま"
      "です。"),
    H("questpersonal", Quest,
      "Return to a normal spatial screen that follows your head, one metre ahead at a readable "
      "angular size. This leaves fixed wall framing and restores normal camera preferences. "
      "Existing wall anchors remain stored.",
      "Revient à un écran qui suit la tête, à un mètre, avec une taille angulaire lisible. Quitte "
      "le cadrage mural fixe et retrouve les caméras normales. Les ancrages enregistrés sont "
      "conservés.",
      "Vuelve a una pantalla que sigue la cabeza, a un metro y con tamaño legible. Sale del "
      "encuadre mural y recupera las cámaras normales. Conserva los anclajes guardados.",
      "頭に追従する通常のスクリーンへ戻します。1m前方で見やすい大きさにします。壁の固定表示を解除し"
      "、通常のカメラ設定を使います。保存したアンカーは保持します。"),
    H("questar", Quest,
      "Show the room around the game using Quest reconstruction passthrough. Camera images are "
      "handled by the headset compositor. Disable for a dark VR cinema.",
      "Affiche la pièce autour du jeu avec le passthrough du Quest. Les images sont gérées par le "
      "compositeur du casque. Désactive pour un cinéma VR sombre.",
      "Muestra la habitación alrededor del juego con passthrough de Quest. El compositor del visor "
      "gestiona las imágenes. Desactívalo para cine oscuro en RV.",
      "Questのパススルーで周囲の部屋を表示します。画像はヘッドセットの合成処理が扱います。無効にす"
      "ると暗いVRシネマになります。"),
    H("questanchor", Quest,
      "Room-pinned stays still as your head moves. Head-following keeps the screen ahead. "
      "Right-stick click recenters at your current position and horizontal viewing direction.",
      "Fixé dans la pièce reste immobile quand ta tête bouge. Suivre la tête garde l’écran devant "
      "toi. Le clic du stick droit recentre selon ta position et ta direction horizontale.",
      "Fija mantiene la pantalla quieta al mover la cabeza. Seguir la cabeza la mantiene delante. "
      "La palanca derecha recentra según tu posición y dirección horizontal.",
      "部屋固定では頭を動かしても同じ位置に残ります。頭に追従では常に正面に表示します。右スティック"
      "押下で現在の位置と視線の正面へ戻します。"),
    H("questwidth", Quest,
      "Set screen width from 80 to 800 cm. Height follows the canvas aspect ratio. Both grips can "
      "resize a room-pinned screen as you move your hands apart or together.",
      "Règle la largeur de 80 à 800 cm. La hauteur suit le ratio du canevas. Maintiens les deux "
      "prises et écarte ou rapproche les mains pour redimensionner un écran fixé.",
      "Ajusta el ancho de 80 a 800 cm. La altura sigue el lienzo. Mantén ambos agarres y separa o "
      "acerca las manos para escalar una pantalla fija.",
      "幅を80～800cmで設定します。高さはキャンバスの縦横比に従います。両グリップを押して手の間隔を"
      "変えると固定スクリーンを拡大縮小できます。"),
    H("questdistance", Quest,
      "Place the screen 80–600 cm ahead when recentering. Both grips can move a room-pinned screen "
      "afterward.",
      "Place l’écran de 80 à 600 cm devant toi lors du recentrage. Les deux prises permettent "
      "ensuite de déplacer un écran fixé.",
      "Coloca la pantalla a 80–600 cm al recentrar. Después puedes mover una pantalla fija con "
      "ambos agarres.",
      "再センタリング時に正面80～600cmへ配置します。部屋固定では両グリップでも移動できます。"),
    H("questheight", Quest,
      "Offset the screen centre from eye height. Negative values lower it for tabletop play. "
      "Recenter after changing between seated and standing play.",
      "Décale le centre par rapport aux yeux. Les valeurs négatives abaissent l’écran en table. "
      "Recentre après un changement entre la position assise et debout.",
      "Desplaza el centro respecto a los ojos. Valores negativos bajan la pantalla para mesa. "
      "Recentra al cambiar entre sentado y de pie.",
      "目の高さに対する中心位置を調整します。負の値でテーブル向けに下げます。座る・立つを切り替えた"
      "ら再センタリングしてください。"),
    H("questtilt", Quest,
      "Tilt the screen toward a tabletop. The physical panel changes orientation while the "
      "cartridge simulation stays the same.",
      "Incline l’écran en table. Cela change l’orientation physique de l’écran tout en conservant "
      "la simulation de la cartouche.",
      "Inclina la pantalla como mesa. Cambia la orientación física y conserva la simulación del "
      "cartucho.",
      "スクリーンをテーブル状に傾けます。空間内の向きが変わり、カートリッジの動作は保たれます。"),
    H("questresolution", Quest,
      "Choose 720p, 1080p, 1440p or an ultra-wide 48:9 canvas. Larger canvases cost more GPU work "
      "and memory. Pixel 1:1 describes this canvas; headset optics and composition resample the "
      "image.",
      "Choisis 720p, 1080p, 1440p ou 48:9. Les grands canevas sollicitent davantage le GPU et la "
      "mémoire. Le 1:1 concerne ce canevas ; les optiques et la composition rééchantillonnent "
      "l’image.",
      "Elige 720p, 1080p, 1440p o 48:9. Los lienzos mayores usan más GPU y memoria. El 1:1 "
      "describe el lienzo; óptica y composición remuestrean la imagen.",
      "720p・1080p・1440p・48:9を選びます。大きいほどGPUとメモリを使います。1:"
      "1はキャンバス内の比率で、光学系と合成処理が画像を再標本化します。"),
    H("questtouch", Quest,
      "Left stick moves; A/right trigger jumps; B starts/pauses; Y opens options; X changes "
      "overview or opens a guide. Switch Touch off to give a Bluetooth gamepad the first player "
      "slot.",
      "Stick gauche : mouvement ; A/gâchette droite : saut ; B : pause ; Y : options ; X : vue "
      "générale ou guide. Désactive Touch pour donner le joueur 1 à une manette Bluetooth.",
      "Palanca izquierda: mover; A/gatillo derecho: saltar; B: pausa; Y: opciones; X: vista "
      "general o guía. Desactiva Touch para asignar jugador 1 al mando Bluetooth.",
      "左スティック移動、A／右トリガーでジャンプ、Bで開始・一時停止、Yで設定、Xで全体表示・ガイド。"
      "Bluetoothゲームパッドをプレイヤー1にするにはTouchを無効にします。"),
    H("questhaptics", Quest,
      "Adjust the short vibration on right-pointer menu clicks. Zero disables it. Game sound and "
      "cartridge controls remain independent.",
      "Règle la brève vibration lors des clics du pointeur droit. Zéro la désactive. Le son et les "
      "commandes de la cartouche restent indépendants.",
      "Ajusta la vibración corta al pulsar con el puntero derecho. Cero la desactiva. Audio y "
      "controles del cartucho son independientes.",
      "右ポインターで選択したときの短い振動を調整します。0で無効。ゲーム音声とカートリッジの操作は"
      "独立しています。"),
    H("questflip", Quest,
      "Reverse the image vertically as a compatibility adjustment. Leave off for the normal "
      "SDL/OpenXR orientation.",
      "Retourne l’image verticalement pour la compatibilité. Laisse désactivé pour l’orientation "
      "SDL/OpenXR normale.",
      "Invierte la imagen verticalmente como ajuste de compatibilidad. Déjalo desactivado para la "
      "orientación SDL/OpenXR normal.",
      "互換性調整として上下反転します。通常のSDL/OpenXR表示では無効のままにしてください。"),
    H("questrecenter", Quest,
      "Place the screen ahead at your current head position and horizontal viewing direction, "
      "preserving size and tilt. Right-stick click does the same during play.",
      "Replace l’écran devant ta position et ta direction horizontale actuelles en gardant la "
      "taille et l’inclinaison. Le clic du stick droit fait pareil en jeu.",
      "Coloca la pantalla delante de tu posición y dirección horizontal, conservando tamaño e "
      "inclinación. La palanca derecha hace lo mismo al jugar.",
      "大きさと傾きを保って現在の頭の位置と水平な視線の正面へ戻します。プレイ中の右スティック押下で"
      "も同じ操作ができます。"),
    H("questcinema", Quest,
      "Use a room-pinned screen 320 cm wide at 240 cm distance, level with the eyes and without "
      "tilt. Other game and save options are retained.",
      "Écran fixé de 320 cm de large à 240 cm, à hauteur des yeux et sans inclinaison. Les autres "
      "réglages du jeu et des sauvegardes sont conservés.",
      "Pantalla fija de 320 cm a 240 cm, a la altura de los ojos y sin inclinación. Conserva los "
      "demás ajustes y guardados.",
      "幅320cm、距離240cm、目の高さ、傾きなしの部屋固定スクリーンにします。他のゲームとセーブ設定は"
      "保ちます。"),
    H("questtabletop", Quest,
      "Use a 220 cm screen at 140 cm distance, 60 cm below eye height with a 65-degree tilt. "
      "Whole-stage view is selected so the terrain can be explored as a board.",
      "Écran de 220 cm à 140 cm, 60 cm sous les yeux et incliné de 65 degrés. Le niveau entier est "
      "sélectionné pour explorer le terrain comme un plateau.",
      "Pantalla de 220 cm a 140 cm, 60 cm bajo los ojos e inclinada 65 grados. Selecciona el nivel "
      "completo para ver el terreno como tablero.",
      "幅220cm、距離140cm、目より60cm下、65度の傾斜にします。ステージ全体を盤面のように見渡す表示へ"
      "切り替えます。"),
    H("hintcorner", Reminder,
      "Put the compact reminder and status cards in any screen corner. The default is bottom "
      "right, at the outside edge even on ultrawide displays. The player close-up moves clear of "
      "these cards.",
      "Place l’indication compacte et les états dans le coin voulu. Par défaut : en bas à droite, "
      "au bord extérieur même en ultra-large. La vue rapprochée évite ces indications.",
      "Coloca el aviso y los estados en cualquier esquina. Por defecto: abajo a la derecha, en el "
      "borde exterior incluso en pantallas ultraanchas. La vista cercana evita los avisos.",
      "操作案内と状態表示を画面の四隅から選びます。初期位置は超横長画面でも外側の右下です。プレイヤ"
      "ーの小画面は重ならない位置へ移動します。"),
    H("hintdetail", Reminder,
      "Compact shows the menu shortcuts on one line. Active input shows only the keyboard or "
      "controller currently in use, with your chosen button labels. Detailed adds rewind bindings "
      "and available history.",
      "Compact affiche les deux commandes du menu sur une ligne. Commande utilisée affiche le "
      "clavier ou la manette en cours avec les boutons choisis. Détaillé ajoute le rewind et "
      "l’historique disponible.",
      "Compacto muestra ambos controles del menú. Control activo muestra solo el teclado o mando "
      "usado, con las etiquetas elegidas. Detallado añade el rebobinado y su historial disponible.",
      "コンパクトは両方のメニュー操作、使用中のみはキーボードかコントローラーの操作を表示します。詳"
      "細は巻き戻しの操作と残り時間も表示します。"),
    H("hintscale", Reminder,
      "Resize the corner overlays from 60% to 180%. Their small default text is independent of "
      "menu and game HUD size. Long text wraps inside the screen, including tiny and portrait "
      "windows.",
      "Règle les indications de 60 à 180 %. Leur petite taille par défaut est indépendante du menu "
      "et du HUD. Les textes longs restent dans l’écran, même petit ou vertical.",
      "Ajusta los avisos del 60 al 180 %. Su texto pequeño es independiente del menú y del HUD. "
      "Los textos largos se adaptan al ancho de la ventana, incluso pequeña o vertical.",
      "案内表示を60～180%"
      "に調整します。初期文字サイズはメニューやHUDとは独立しています。長い文は小画面や縦画面でも画"
      "面内で折り返します。"),
    H("hintopacity", Reminder,
      "Adjust the dark backing of the reminder, notifications and status cards. Text stays opaque "
      "for readability. High contrast uses a nearly solid backing regardless of this setting.",
      "Règle le fond sombre des indications, notifications et états. Le texte reste opaque. Le "
      "contraste élevé utilise un fond presque uni, quel que soit ce réglage.",
      "Ajusta el fondo oscuro de avisos, notificaciones y estados. El texto sigue opaco. Alto "
      "contraste usa un fondo casi sólido independientemente de este ajuste.",
      "操作案内・通知・状態表示の背景を調整します。文字は読みやすい不透明のままです。高コントラスト"
      "ではこの値に関わらず背景を濃くします。"),
    H("hintmargin", Reminder,
      "Leave 0–64 window pixels between the overlay cards and the screen edges. This also helps "
      "with display overscan. Margins are bounded automatically on tiny windows.",
      "Laisse de 0 à 64 pixels entre les indications et le bord. Utile aussi si l’écran coupe ses "
      "bords. La marge s’adapte aux toutes petites fenêtres.",
      "Deja de 0 a 64 píxeles entre los avisos y el borde. También sirve si la pantalla recorta "
      "los bordes. El margen se limita en ventanas pequeñas.",
      "表示と画面端の間に0～64ピクセルの余白を設けます。画面端が切れる場合にも便利です。小画面では"
      "余白を自動的に制限します。"),
    H("hintseconds", Reminder,
      "Zero keeps the reminder visible. Otherwise it hides after the chosen number of seconds of "
      "play and reappears when you resume from the menu. Select/View, Esc and F10 always open "
      "options, even when hidden.",
      "Zéro garde l’indication visible. Sinon elle se masque après le délai choisi et revient à la "
      "reprise depuis le menu. Select/View, Échap et F10 fonctionnent toujours.",
      "Cero mantiene el aviso visible. Otro valor lo oculta tras ese tiempo y vuelve al salir del "
      "menú. Select/View, Esc y F10 siempre abren opciones aunque esté oculto.",
      "0では常に表示します。それ以外は指定秒数で隠れ、メニューから再開すると再表示します。非表示で"
      "もSelect/View・Esc・F10で設定を開けます。"),
    H("noticeseconds", Reminder,
      "Choose how long status notifications remain during play. Zero keeps them until you dismiss "
      "them in the menu. Messages remain available in the menu after their overlay expires; click "
      "a notification to open it. Long overlays are clipped, with full text on hover or in the "
      "menu.",
      "Choisis la durée des notifications en jeu. Zéro les garde jusqu’à leur fermeture dans le "
      "menu. Après le délai, le texte reste dans le menu. Clique pour l’ouvrir. Les longs textes "
      "sont limités en jeu, complets au survol ou dans le menu.",
      "Elige la duración de las notificaciones. Cero las mantiene hasta cerrarlas en el menú. El "
      "mensaje sigue en el menú tras desaparecer; púlsalo para abrirlo. Los textos largos se "
      "limitan en juego y se leen completos al apuntarlos o en el menú.",
      "通知の表示秒数を選びます。0ではメニューで閉じるまで表示します。消えた通知もメニューで確認で"
      "きます。クリックでメニューを開きます。長文の全文はマウスを重ねるかメニューで読めます。"),
    H("rewindindicator", Rewind,
      "Show a small status card only while rewinding, including remaining history. It works even "
      "if the menu reminder is hidden or timed out. Detailed reminders already contain this "
      "information.",
      "Affiche un petit état uniquement pendant le rewind, avec l’historique restant. Il "
      "fonctionne même si l’indication du menu est masquée. L’indication détaillée le contient "
      "déjà.",
      "Muestra un estado pequeño solo al rebobinar, con el historial restante. Funciona aunque el "
      "aviso esté oculto. Los avisos detallados ya incluyen esa información.",
      "巻き戻し中だけ残り時間を小さく表示します。操作案内が非表示でも使えます。詳細案内に同じ情報が"
      "ある場合は重複しません。"),
    H("smoothmap", View,
      "Average several terrain samples and smooth reduced maps to reduce shimmer. Exact 1:1 and "
      "magnified pixel views stay sharp. Disable for faster nearest-pixel maps.",
      "Moyenne plusieurs échantillons du terrain et lisse les cartes réduites pour limiter le "
      "scintillement. Le 1:1 et les pixels agrandis restent nets. Désactive pour accélérer les "
      "cartes.",
      "Promedia muestras del terreno y suaviza mapas reducidos para reducir parpadeos. Las vistas "
      "1:1 y ampliadas conservan píxeles nítidos. Desactívalo para mapas más rápidos.",
      "地形の複数点を平均して縮小図のちらつきを抑えます。等倍と拡大表示の画素は鮮明なままです。無効"
      "にすると縮小図を高速化できます。"),
    H("language", Controller,
      "Automatic follows English, French, Spanish or Japanese system preferences. English is the "
      "fallback.",
      "Automatique suit la langue du système : anglais, français, espagnol ou japonais. L’anglais "
      "sert de repli.",
      "Automático sigue el idioma del sistema: inglés, francés, español o japonés. El inglés es la "
      "alternativa.",
      "自動では英語・フランス語・スペイン語・日本語のシステム設定に従い、それ以外は英語になります"
      "。"),
    H("preset", Performance,
      "Lightweight reduces overview sampling and background work. Balanced is recommended; Ultra "
      "keeps every output pixel. Your camera and view stay selected.",
      "Léger réduit l’échantillonnage de la vue globale et le travail du fond. Équilibré est "
      "conseillé ; Ultra calcule chaque pixel de sortie. Le cadrage choisi est conservé.",
      "Ligero reduce el muestreo de la vista completa y el trabajo del fondo. Equilibrado es "
      "recomendable; Ultra calcula cada píxel de salida. Conserva la vista elegida.",
      "軽量は全体図の解像度と背景処理を抑えます。通常はバランスを推奨。ウルトラは出力の全画素を計算"
      "し、選択した視点は保持します。"),
    H("view", View,
      "Pixel world uses physical display pixels. Panorama fits a chosen world height. Overview "
      "fits the entire supported map. Original framing preserves the cartridge screen.",
      "Pixels utilise les pixels physiques de l’écran. Panorama cadre la hauteur choisie. Vue "
      "globale affiche toute la carte compatible. Original conserve le cadre de la cartouche.",
      "Píxeles usa los píxeles físicos de la pantalla. Panorama ajusta la altura elegida. Vista "
      "completa muestra todo el mapa compatible. Original conserva el encuadre del cartucho.",
      "ピクセル表示は画面の物理画素を使い、パノラマは指定した高さに合わせます。全体図は対応マップ全"
      "域、オリジナルはカートリッジ本来の画面です。"),
    H("overviewstyle", View,
      "Automatic chooses a terrain diagram for streamed Sonic 3 art and cartridge art elsewhere. "
      "The diagram describes terrain, not collision. Live actors remain visible.",
      "Automatique choisit un schéma pour les graphismes chargés progressivement de Sonic 3, et "
      "les graphismes de la cartouche ailleurs. Ce schéma décrit le terrain, pas les collisions.",
      "Automático usa un esquema para los gráficos de Sonic 3 cargados por zonas y gráficos del "
      "cartucho en los demás. El esquema representa terreno, no colisiones.",
      "自動では逐次読み込みを使うソニック3を地形図、それ以外をゲーム画像で表示します。地形図は当た"
      "り判定ではありません。動くキャラクターも表示します。"),
    H("zoom", Zoom,
      "1× with a 1× zoom-out divisor is one game pixel per physical screen pixel. Higher "
      "magnification shows less terrain and larger characters.",
      "1× avec un diviseur de 1× donne un pixel du jeu par pixel physique. Un agrandissement "
      "supérieur montre moins de terrain et des personnages plus grands.",
      "1× con divisor 1× equivale a un píxel del juego por píxel físico. Más aumento muestra menos "
      "terreno y personajes mayores.",
      "拡大率1倍・縮小除数1倍でゲームの1画素が画面の1画素になります。拡大すると見える範囲が狭まり、"
      "キャラクターが大きくなります。"),
    H("zoomout", Zoom,
      "Divide magnification by 1, 2, 4 or 8 to see farther, including below 1:1. Tiny details can "
      "disappear when several game pixels share one display pixel.",
      "Divise l’agrandissement par 1, 2, 4 ou 8 pour voir plus loin, même sous le 1:1. Les petits "
      "détails peuvent disparaître lors de la réduction.",
      "Divide el aumento por 1, 2, 4 u 8 para ver más lejos, incluso por debajo de 1:1. Los "
      "detalles pequeños pueden perderse al reducir.",
      "拡大率を1・2・4・8で割り、等倍より広い範囲も表示できます。縮小すると小さな物が見えにくくなる"
      "場合があります。"),
    H("pixelreset", Zoom,
      "Select Pixel world and restore exact 1:1 scaling in one action. The camera keeps its "
      "current follow mode.",
      "Passe en mode Pixels et rétablit le 1:1 en une action. Le mode de suivi de la caméra est "
      "conservé.",
      "Activa Píxeles y restaura la escala exacta 1:1. Conserva el modo de seguimiento de la "
      "cámara.",
      "ピクセル表示に切り替え、等倍に戻します。カメラの追従モードは維持します。"),
    H("panoramaheight", Zoom,
      "World pixels visible vertically in Panorama. Increase this to see more of a tall Spinball "
      "board; the screen aspect ratio determines horizontal coverage.",
      "Nombre de pixels du monde visibles en hauteur en Panorama. Augmente cette valeur pour voir "
      "davantage d’un plateau de Spinball ; la largeur suit le format de l’écran.",
      "Píxeles del mundo visibles en vertical en Panorama. Auméntalos para ver más del tablero de "
      "Spinball; la anchura sigue la proporción de la pantalla.",
      "パノラマで縦に表示するゲーム画素数です。増やすとスピンボールの縦長ボードを広く表示でき、横幅"
      "は画面比率に従います。"),
    H("mapextension", View,
      "Decode all four Spinball boards and all 22 Sonic 3D Blast terrain maps directly from your "
      "cartridge. Distant scenery is cached; moving actors stay native to the active area.",
      "Décode les quatre plateaux de Spinball et les 22 cartes de Sonic 3D Blast depuis ta "
      "cartouche. Le décor éloigné est mis en cache ; les acteurs mobiles restent dans la zone "
      "active.",
      "Decodifica los cuatro tableros de Spinball y los 22 mapas de Sonic 3D Blast desde tu "
      "cartucho. El terreno lejano se almacena en caché; los actores móviles siguen en el área "
      "activa.",
      "カートリッジからスピンボールの全4ボードとソニック3Dの全22地形を復元します。遠景はキャッシュ"
      "し、動く物は本来の活動範囲に従います。"),
    H("fullscreen", View,
      "Borderless fullscreen uses the selected display’s desktop resolution. Alt+Enter switches "
      "back to a window.",
      "Le plein écran sans bordure utilise la résolution du bureau de l’écran choisi. Alt+Entrée "
      "revient en fenêtre.",
      "Pantalla completa sin bordes usa la resolución del monitor elegido. Alt+Intro vuelve a la "
      "ventana.",
      "ボーダーレス全画面は選択したモニターの解像度を使います。Alt+Enterでウィンドウに戻れます。"),
    H("span", View,
      "Span the bounding rectangle of all displays. Monitors with different sizes or offsets can "
      "leave invisible gaps; use one display to avoid those gaps.",
      "Couvre le rectangle englobant tous les écrans. Des tailles ou positions différentes peuvent "
      "créer des zones invisibles ; un seul écran évite ces espaces.",
      "Abarca el rectángulo de todos los monitores. Distintos tamaños o posiciones pueden dejar "
      "huecos invisibles; un monitor evita esos huecos.",
      "全モニターを囲む範囲に表示します。大きさや位置が異なると見えない隙間が生じるため、その場合は"
      "1台を選んでください。"),
    H("display", View,
      "Choose the fullscreen monitor. Current display follows the window’s monitor; spanning "
      "overrides this choice.",
      "Choisit l’écran du plein écran. Écran actuel suit celui de la fenêtre ; l’étendue sur tous "
      "les écrans prime sur ce choix.",
      "Elige el monitor de pantalla completa. Monitor actual sigue a la ventana; abarcar todos "
      "tiene prioridad.",
      "全画面に使うモニターを選びます。「現在の画面」はウィンドウのあるモニターです。全画面をまたぐ"
      "設定が優先されます。"),
    H("menulayout", Hud,
      "Fullscreen fills the display while keeping controls in a readable central area. Compact "
      "uses a smaller floating panel.",
      "Plein écran remplit l’affichage avec les commandes au centre. Compact utilise un panneau "
      "flottant plus petit.",
      "Completo llena la pantalla con controles centrados y legibles. Compacto usa un panel "
      "flotante más pequeño.",
      "全画面メニューは中央に読みやすく配置します。コンパクトは小さなパネルを使います。"),
    H("uiscale", Hud,
      "Enlarge menu text and controls independently of game pixels. Automatic adapts to window "
      "height; small screens can scroll every page.",
      "Agrandit le texte et les commandes indépendamment du jeu. Automatique suit la hauteur de la "
      "fenêtre ; toutes les pages défilent sur petit écran.",
      "Amplía texto y controles sin cambiar los píxeles del juego. Automático se adapta a la "
      "altura; las páginas se desplazan en pantallas pequeñas.",
      "ゲーム画像とは別に文字と操作部を拡大します。自動は画面の高さに合わせ、小さな画面でも各ページ"
      "をスクロールできます。"),
    H("contrast", Hud,
      "Increase text, focus and control contrast. The selected gamepad control stays clearly "
      "outlined.",
      "Renforce le contraste du texte, du focus et des commandes. La commande sélectionnée à la "
      "manette reste bien encadrée.",
      "Aumenta el contraste del texto, foco y controles. Resalta claramente el control "
      "seleccionado con el mando.",
      "文字・選択枠・操作部のコントラストを上げ、ゲームパッドで選んだ項目を見やすくします。"),
    H("vsync", Performance,
      "Synchronize presentation with the display to reduce tearing. Cartridge timing stays "
      "independent of display refresh rate.",
      "Synchronise l’affichage avec l’écran pour limiter les déchirures. La cadence de la "
      "cartouche reste indépendante de celle de l’écran.",
      "Sincroniza la imagen con el monitor para reducir cortes. La velocidad del cartucho es "
      "independiente de la frecuencia de pantalla.",
      "画面更新をモニターに同期してティアリングを抑えます。ゲームの時間進行はモニターの周波数に依存"
      "しません。"),
    H("nearest", Zoom,
      "Keep crisp nearest-neighbour pixels and integer scaling for original framing. Disable for "
      "smooth resizing of the native screen.",
      "Garde des pixels nets et un agrandissement entier du cadre original. Désactive pour lisser "
      "le redimensionnement du jeu natif.",
      "Mantiene píxeles nítidos y escala entera en el encuadre original. Desactívalo para suavizar "
      "el tamaño de la pantalla nativa.",
      "オリジナル画面を最近傍・整数倍率で鮮明に表示します。無効にすると滑らかに拡大縮小します。"),
    H("nativescale", Zoom,
      "Original-frame magnification; zero fits automatically. Pixel world uses its own "
      "magnification. Large values are limited to the available screen.",
      "Agrandissement du cadre original ; zéro ajuste automatiquement. Le mode Pixels utilise son "
      "propre zoom. Les grandes valeurs restent limitées par l’écran.",
      "Aumento del encuadre original; cero ajusta automáticamente. Píxeles usa su propio aumento. "
      "Los valores grandes se limitan a la pantalla.",
      "オリジナル画面の倍率です。0は自動調整。ピクセル表示は別の倍率を使い、大きすぎる値は画面に収"
      "めます。"),
    H("workers", Performance,
      "Terrain-rendering CPU threads. Zero selects up to eight automatically when parallel "
      "rendering is available. More threads do not always improve speed.",
      "Nombre de fils CPU pour le terrain. Zéro en choisit jusqu’à huit si le rendu parallèle est "
      "disponible. En ajouter n’accélère pas toujours le rendu.",
      "Hilos de CPU para el terreno. Cero elige hasta ocho si hay renderizado paralelo. Más hilos "
      "no siempre dan más velocidad.",
      "地形描画のCPUスレッド数です。0では並列描画が利用可能な場合に最大8本を自動選択します。増やし"
      "ても速くなるとは限りません。"),
    H("soft", Performance,
      "Use the CPU renderer after restarting the app. This can help with driver problems; large "
      "windows may be slower.",
      "Utilise le rendu CPU au prochain lancement. Utile en cas de problème de pilote ; les "
      "grandes fenêtres peuvent être plus lentes.",
      "Usa el renderizador de CPU al reiniciar la aplicación. Ayuda con problemas de "
      "controladores; las ventanas grandes pueden ir más lentas.",
      "次回起動からCPU描画を使います。ドライバーの問題を回避できますが、大きな画面では遅くなる場合"
      "があります。"),
    H("scanlines", View,
      "Darken alternating screen lines for a CRT-like texture. Zero disables the effect; it does "
      "not change the game’s resolution.",
      "Assombrit certaines lignes pour évoquer un écran cathodique. Zéro désactive l’effet sans "
      "changer la résolution du jeu.",
      "Oscurece líneas para dar una textura de CRT. Cero desactiva el efecto sin cambiar la "
      "resolución del juego.",
      "走査線を暗くしてブラウン管風にします。0で無効になり、ゲーム解像度自体は変わりません。"),
    H("performance", Performance,
      "Show both cartridge ticks per second and actual presented frames per second. They measure "
      "different work, especially in very large views.",
      "Affiche séparément la cadence de la cartouche et les images réellement présentées par "
      "seconde. Ces mesures diffèrent, surtout dans les très grandes vues.",
      "Muestra por separado los ciclos del cartucho y las imágenes presentadas por segundo. Son "
      "medidas distintas, sobre todo en vistas muy grandes.",
      "ゲームの更新回数と実際の描画フレーム数を別々に表示します。特に超広域表示では異なる値になりま"
      "す。"),
    H("defaults", Save,
      "Restore application defaults after confirmation. Your imported cartridges and saved games "
      "are kept.",
      "Rétablit les réglages par défaut après confirmation. Les cartouches importées et les "
      "sauvegardes sont conservées.",
      "Restaura los ajustes predeterminados tras confirmar. Conserva los cartuchos importados y "
      "las partidas guardadas.",
      "確認後にアプリ設定を初期値へ戻します。取り込んだカートリッジとセーブは保持されます。"),
    H("showstage", View,
      "Fit the whole supported terrain and enable the player marker. Mean Bean, Blue Sphere and "
      "unsupported scenes retain their original complete screen.",
      "Cadre tout le terrain compatible et active le repère du joueur. Mean Bean, Blue Sphere et "
      "les scènes non compatibles conservent leur écran d’origine complet.",
      "Ajusta todo el terreno compatible y activa el marcador del jugador. Mean Bean, Blue Sphere "
      "y escenas no compatibles conservan su pantalla original completa.",
      "対応地形全体を収め、プレイヤーマーカーを表示します。Mean Bean・Blue "
      "Sphere・非対応場面は本来の画面を維持します。"),
    H("focusplayer", Camera,
      "Switch to Pixel world and centre the camera on the player. Change magnification if the "
      "player is too small.",
      "Passe en mode Pixels et centre la caméra sur le joueur. Ajuste le zoom si le personnage est "
      "trop petit.",
      "Activa Píxeles y centra la cámara en el jugador. Ajusta el aumento si el personaje es "
      "demasiado pequeño.",
      "ピクセル表示に切り替え、プレイヤーを中央に追従します。小さく見える場合は倍率を変えてください"
      "。"),
    H("playerinset", Hud,
      "Keep a live original-screen close-up in stage overview, including authentic counters and "
      "animations. Useful when the full map makes the player tiny.",
      "Ajoute une vue rapprochée native dans la vue globale, avec les compteurs et animations "
      "authentiques. Utile quand le personnage devient minuscule.",
      "Añade una vista nativa en directo en la vista completa, con contadores y animaciones "
      "originales. Útil cuando el personaje se ve diminuto.",
      "全体図に本来の画面の拡大表示を添え、カウンターやアニメーションを確認できます。プレイヤーが小"
      "さい場合に便利です。"),
    H("enemypreview", View,
      "Preview inactive placed enemies in supported Sonic platform stages. These transparent "
      "previews do not activate enemies or change collisions; other game formats are not "
      "predicted.",
      "Montre les ennemis placés inactifs dans les stages Sonic compatibles. Ces aperçus ne "
      "déclenchent ni ennemis ni collisions ; les autres formats de jeu ne sont pas prédits.",
      "Muestra enemigos colocados inactivos en fases Sonic compatibles. Las vistas transparentes "
      "no activan enemigos ni colisiones; no predicen otros formatos de juego.",
      "対応するソニックのステージで未出現の配置敵を半透明表示します。敵や当たり判定は起動しません。"
      "他形式のゲームは予測対象外です。"),
    H("bosspreview", View,
      "Show supported boss spawn positions. A marker is used when cartridge sprite art cannot be "
      "decoded safely.",
      "Montre les positions d’apparition des boss compatibles. Un repère remplace le sprite si son "
      "décodage n’est pas fiable.",
      "Muestra posiciones de aparición de jefes compatibles. Usa un marcador cuando no puede "
      "decodificar el sprite con seguridad.",
      "対応するボスの出現位置を表示します。画像を安全に復元できない場合はマーカーを使います。"),
    H("scriptpreview", View,
      "Show supported scripted spawns and projectile origins. Variable attacks are marked at their "
      "origin, not at a guessed future landing position.",
      "Montre les apparitions scriptées et origines de projectiles compatibles. Une attaque "
      "variable est signalée à son origine, sans inventer son point d’arrivée.",
      "Muestra apariciones programadas y orígenes de proyectiles compatibles. Los ataques "
      "variables se marcan en su origen, sin adivinar dónde caerán.",
      "対応するイベント出現と弾の発生元を表示します。変動する攻撃は着地点を推測せず、発生元を示しま"
      "す。"),
    H("enemystyle", View,
      "Choose cartridge ghost sprites, original diamond markers or both. Markers remain readable "
      "when overview shrinks sprites below a few pixels.",
      "Choisit les sprites transparents, des losanges de repérage ou les deux. Les repères restent "
      "lisibles quand les sprites deviennent minuscules.",
      "Elige sprites transparentes, marcadores en rombo o ambos. Los marcadores siguen siendo "
      "legibles cuando la vista completa reduce los sprites.",
      "半透明画像・ひし形マーカー・両方から選びます。全体図で画像が小さくなってもマーカーは判別でき"
      "ます。"),
    H("enemyopacity", View,
      "Set how visible inactive enemy previews are. Active native enemies keep their original "
      "appearance.",
      "Règle la visibilité des aperçus d’ennemis inactifs. Les ennemis actifs gardent leur "
      "apparence d’origine.",
      "Ajusta la visibilidad de las vistas previas de enemigos inactivos. Los activos conservan su "
      "aspecto original.",
      "未出現敵の半透明表示の濃さを変えます。活動中の敵は本来の表示を保ちます。"),
    H("spawntint", View,
      "Amber identifies enemies, red bosses and purple scripted origins. Disable tint to keep "
      "ghost sprites closer to cartridge colours.",
      "L’ambre identifie les ennemis, le rouge les boss et le violet les origines scriptées. "
      "Désactive la teinte pour conserver davantage les couleurs natives.",
      "Ámbar identifica enemigos, rojo jefes y morado orígenes programados. Desactívalo para "
      "conservar mejor los colores originales.",
      "敵は琥珀色、ボスは赤、イベント発生元は紫です。無効にすると半透明画像を本来の色に近づけます"
      "。"),
    H("spawnlabels", Hud,
      "Label bosses and variable spawn origins, with overlap avoidance. Labels are limited on "
      "small screens to keep the map readable.",
      "Étiquette les boss et les origines variables en évitant les chevauchements. Leur nombre est "
      "limité sur petit écran pour garder la carte lisible.",
      "Etiqueta jefes y orígenes variables evitando solapamientos. Limita su número en pantallas "
      "pequeñas para mantener la legibilidad.",
      "ボスと変動する発生元にラベルを付け、重なりを避けます。小さい画面では数を制限して見やすさを保"
      "ちます。"),
    H("fitspawns", Camera,
      "Expand overview bounds to include supported distant spawn locations. Disable to frame only "
      "the detected terrain.",
      "Élargit le cadrage global pour inclure les apparitions éloignées compatibles. Désactive "
      "pour cadrer seulement le terrain détecté.",
      "Amplía los límites de la vista completa para incluir apariciones lejanas compatibles. "
      "Desactívalo para encuadrar solo el terreno detectado.",
      "対応する遠方の出現位置も全体図に含めます。無効にすると検出した地形だけを収めます。"),
    H("cameramode", Camera,
      "Bounded follows within the map; Centred follows freely. Fixed stays still, Free allows "
      "manual panning, and Whole stage fits the map.",
      "Bornée suit dans la carte ; Centrée suit librement. Fixe reste immobile, Libre permet le "
      "déplacement manuel, et Niveau entier cadre toute la carte.",
      "Limitada sigue dentro del mapa; Centrada sigue libremente. Fija permanece inmóvil, Libre "
      "permite desplazarse y Fase completa ajusta el mapa.",
      "範囲内追従・中央追従・固定・自由移動・ステージ全体から選びます。自由では手動で視点を動かせま"
      "す。"),
    H("freeze", Camera,
      "Lock the current camera position. Gameplay continues inside the fixed view; use Follow to "
      "track the player again.",
      "Verrouille la position actuelle de la caméra. Le jeu continue dans ce cadre ; Suivre "
      "réactive le suivi du joueur.",
      "Bloquea la posición actual de la cámara. El juego continúa; Seguir vuelve a seguir al "
      "jugador.",
      "現在のカメラ位置を固定します。ゲームは進行し、「追従」でプレイヤー追従に戻せます。"),
    H("follow", Camera,
      "Return to Pixel world with bounded player following. The selected zoom is preserved.",
      "Revient en mode Pixels avec suivi borné du joueur. Le zoom choisi est conservé.",
      "Vuelve a Píxeles con seguimiento limitado del jugador. Conserva el aumento elegido.",
      "ピクセル表示と範囲内追従に戻ります。設定した倍率は保持します。"),
    H("panspeed", Camera,
      "Movement speed for the free camera. Use the right stick, or WASD while the free-camera mode "
      "is selected.",
      "Vitesse de déplacement de la caméra libre. Utilise le stick droit, ou WASD en mode caméra "
      "libre.",
      "Velocidad de la cámara libre. Usa el stick derecho o WASD con la cámara libre seleccionada.",
      "自由カメラの移動速度です。右スティック、または自由モードでWASDを使います。"),
    H("hud", Hud,
      "Keep counters in the centre monitor’s safe area, near the player, at the screen edge or "
      "hidden. Spinball uses its authentic score strip; 3D Blast shows rings and lives.",
      "Place les compteurs sur l’écran central, près du joueur, au bord ou les masque. Spinball "
      "garde sa bande de score ; 3D Blast affiche anneaux et vies.",
      "Sitúa contadores en el monitor central, junto al jugador, en el borde u ocultos. Spinball "
      "usa su franja original; 3D Blast muestra anillos y vidas.",
      "HUDを中央モニター内・プレイヤー付近・画面端・非表示から選びます。スピンボールは元の得点欄、3"
      "Dはリングと残機を表示します。"),
    H("hudsize", Hud,
      "Scale counters independently of terrain. Zero chooses a readable size from display height.",
      "Agrandit les compteurs indépendamment du terrain. Zéro choisit une taille lisible selon la "
      "hauteur de l’écran.",
      "Aumenta contadores sin cambiar el terreno. Cero elige un tamaño legible según la altura.",
      "地形とは別にHUDを拡大します。0では画面の高さから読みやすい大きさを選びます。"),
    H("locator", Hud,
      "Circle the player and point toward them if they leave the current view. Especially useful "
      "at 1:1 on several monitors.",
      "Entoure le joueur et indique sa direction hors champ. Particulièrement utile en 1:1 sur "
      "plusieurs écrans.",
      "Rodea al jugador e indica su dirección si sale de la vista. Muy útil en 1:1 con varios "
      "monitores.",
      "プレイヤーを丸で囲み、画面外では方向を示します。複数モニターの等倍表示で特に便利です。"),
    H("minimap", Hud,
      "Show a terrain locator with the camera rectangle and player position. This schematic is a "
      "navigation aid, not a collision map.",
      "Affiche un repère de terrain avec le rectangle de caméra et la position du joueur. Ce "
      "schéma aide à se situer ; ce n’est pas une carte des collisions.",
      "Muestra un esquema del terreno con la cámara y el jugador. Es una ayuda de orientación, no "
      "un mapa de colisiones.",
      "地形の略図にカメラ枠とプレイヤー位置を表示します。移動の目安であり、当たり判定図ではありませ"
      "ん。"),
    H("ghost", Camera,
      "Outline the cartridge’s current 320×224 camera window within the extended terrain.",
      "Dessine le cadre natif 320×224 de la caméra dans le terrain étendu.",
      "Dibuja el encuadre nativo de 320×224 dentro del terreno ampliado.",
      "広域地形の中に本来の320×224カメラ枠を表示します。"),
    H("help", Reminder,
      "Show a small clickable options reminder in the selected screen corner. Set its content, "
      "size and optional hide timer below. Select/View, Esc and F10 always open options. Rewind "
      "status, notifications and performance counters have independent visibility.",
      "Affiche une petite indication cliquable dans le coin choisi. Règle son contenu, sa taille "
      "et son délai ci-dessous. Select/View, Échap et F10 ouvrent toujours le menu. Les états du "
      "rewind, notifications et performances restent indépendants.",
      "Muestra un aviso pequeño en la esquina elegida. Ajusta contenido, tamaño y temporizador "
      "debajo. Select/View, Esc y F10 siempre abren opciones. Rebobinado, notificaciones y "
      "rendimiento tienen visibilidad independiente.",
      "選んだ隅にクリック可能な小さい操作案内を表示します。内容・大きさ・表示時間を調整できます。Se"
      "lect/View・Esc・F10は常に使えます。巻き戻し・通知・性能表示は独立しています。"),
    H("backgroundstyle", Background,
      "Choose stretched blurred scenery, sharp scenery, tiled scenery, a gradient, a dark wash or "
      "black. The terrain and characters stay sharp.",
      "Choisit un décor étiré flou, net, répété, un dégradé, un fond sombre ou noir. Le terrain et "
      "les personnages restent nets.",
      "Elige paisaje estirado difuminado, nítido, repetido, degradado, oscuro o negro. El terreno "
      "y los personajes siguen nítidos.",
      "引き伸ばしたぼかし背景・鮮明・繰り返し・グラデーション・暗色・黒から選びます。地形とキャラク"
      "ターは鮮明なままです。"),
    H("blur", Background,
      "Blur only the extended backdrop. Higher values soften repeated scenery; zero keeps it "
      "sharp.",
      "Floute seulement l’arrière-plan étendu. Une valeur élevée adoucit le décor ; zéro le garde "
      "net.",
      "Difumina solo el fondo ampliado. Valores altos suavizan el paisaje; cero lo mantiene "
      "nítido.",
      "拡張背景だけをぼかします。値が大きいほど柔らかくなり、0では鮮明です。"),
    H("brightness", Background,
      "Dim the backdrop without dimming foreground gameplay. Lower values help separate the "
      "playable terrain from the scenery.",
      "Assombrit le fond sans assombrir le jeu au premier plan. Une valeur basse aide à distinguer "
      "le terrain jouable.",
      "Oscurece el fondo sin oscurecer el juego. Valores bajos ayudan a distinguir el terreno "
      "jugable.",
      "ゲームの手前の表示を変えずに背景を暗くします。低くすると遊べる地形を見分けやすくなります。"),
    H("parallax", Background,
      "Allow scenery to follow the native game camera. Disable to hold one uniform backdrop; fixed "
      "cameras and overview also hold it still.",
      "Autorise le décor à suivre la caméra native. Désactive pour garder un fond uniforme "
      "immobile ; les vues fixes et globales le figent aussi.",
      "Permite que el paisaje siga la cámara nativa. Desactívalo para un fondo uniforme fijo; las "
      "vistas fijas y completas también lo mantienen inmóvil.",
      "背景を本来のカメラに追従させます。無効・固定カメラ・全体図では背景を静止させます。"),
    H("motion", Background,
      "Hold the extended background still to reduce visual motion. Native gameplay animations are "
      "preserved.",
      "Fige l’arrière-plan étendu pour réduire les mouvements visuels. Les animations natives du "
      "jeu sont conservées.",
      "Fija el fondo ampliado para reducir movimiento visual. Conserva las animaciones originales "
      "del juego.",
      "拡張背景を止めて画面の動きを抑えます。ゲーム本来のアニメーションは保持します。"),
    H("volume", Audio,
      "Set game audio volume, including reversed rewind sound. Your operating system’s volume "
      "remains independent.",
      "Règle le volume du jeu, y compris le son inversé du retour arrière. Le volume du système "
      "reste indépendant.",
      "Ajusta el volumen del juego y del sonido inverso al rebobinar. El volumen del sistema es "
      "independiente.",
      "ゲームと巻き戻し逆再生音の音量です。OSの音量とは独立しています。"),
    H("mute", Audio, "Silence game audio without stopping emulation or rewind recording.",
      "Coupe le son sans arrêter le jeu ni l’enregistrement du retour arrière.",
      "Silencia el juego sin detener la emulación ni el historial de rebobinado.",
      "ゲームや巻き戻し記録を止めずに消音します。"),
    H("pausefocus", Controller,
      "Pause when another application has focus. Disable if you want gameplay to continue in the "
      "background.",
      "Met en pause quand une autre application reçoit le focus. Désactive pour laisser le jeu "
      "tourner en arrière-plan.",
      "Pausa cuando otra aplicación tiene el foco. Desactívalo para que el juego continúe en "
      "segundo plano.",
      "別のアプリが選択されると一時停止します。無効にすると背面でもゲームが進みます。"),
    H("padlabels", Controller,
      "Choose Xbox, PlayStation or Nintendo menu labels. Automatic detects the connected "
      "controller; physical button positions stay consistent.",
      "Choisit les indications Xbox, PlayStation ou Nintendo. Automatique détecte la manette ; les "
      "positions physiques restent cohérentes.",
      "Elige indicaciones Xbox, PlayStation o Nintendo. Automático detecta el mando; las "
      "posiciones físicas se mantienen.",
      "Xbox・PlayStation・Nintendoのボタン表記を選びます。自動は接続した機種を判別し、物理位置は共"
      "通です。"),
    H("jumpbuttons", Controller,
      "Choose how the three cartridge buttons map to gamepad face buttons. This also matters for "
      "Spinball flippers and Mean Bean rotation.",
      "Choisit la disposition des trois boutons de la cartouche sur la manette. Cela concerne "
      "aussi les batteurs de Spinball et la rotation de Mean Bean.",
      "Elige cómo se asignan los tres botones del cartucho al mando. También afecta a las palancas "
      "de Spinball y la rotación de Mean Bean.",
      "カートリッジの3ボタンをパッドへ割り当てます。スピンボールのフリッパーやMean "
      "Beanの回転にも影響します。"),
    H("rewindbutton", Rewind,
      "Hold L2/LT by default, or choose L1/LB or R2/RT. The button rewinds at normal speed; "
      "release to continue playing.",
      "Maintiens L2/LT par défaut, ou choisis L1/LB ou R2/RT. Le retour se fait à vitesse normale "
      "; relâche pour reprendre.",
      "Mantén L2/LT por defecto o elige L1/LB o R2/RT. Rebobina a velocidad normal; suelta para "
      "continuar.",
      "初期設定はL2/LT長押しです。L1/LB・R2/RTも選べます。等速で戻り、離すと再開します。"),
    H("quickcontrols", Controller,
      "Hold the right shoulder during play for zoom, overview, camera freeze and capture "
      "shortcuts. Disable to reserve those buttons for gameplay.",
      "Maintiens l’épaule droite en jeu pour les raccourcis de zoom, vue globale, caméra et "
      "capture. Désactive pour réserver les boutons au jeu.",
      "Mantén el hombro derecho al jugar para atajos de zoom, vista completa, cámara y captura. "
      "Desactívalo para reservar esos botones al juego.",
      "プレイ中に右ショルダーを押しながらズーム・全体図・カメラ固定・撮影を操作できます。無効にする"
      "とボタンをゲーム専用にします。"),
    H("deadzone", Controller,
      "Ignore small stick movement around the centre. Increase to prevent drift; decrease for a "
      "more responsive stick.",
      "Ignore les petits mouvements du stick au centre. Augmente pour éviter la dérive ; diminue "
      "pour plus de réactivité.",
      "Ignora movimientos pequeños cerca del centro. Aumenta para evitar deriva; reduce para más "
      "respuesta.",
      "スティック中央付近の微小入力を無視します。大きくするとドリフトを防ぎ、小さくすると反応が早く"
      "なります。"),
    H("lockon", Assist,
      "Attach your imported Sonic & Knuckles cartridge on the next game load. Availability depends "
      "on the imported cartridge combination and supported lock-on data.",
      "Attache ta cartouche Sonic & Knuckles au prochain chargement. La disponibilité dépend des "
      "cartouches importées et des données lock-on compatibles.",
      "Acopla tu cartucho Sonic & Knuckles en la próxima carga. Depende de la combinación "
      "importada y de los datos lock-on compatibles.",
      "次のゲーム読込時に取り込んだソニック＆ナックルズを接続します。組み合わせと対応ロックオンデー"
      "タが必要です。"),
    H("spindash", Assist,
      "Optional Sonic 1 assist: hold Down and tap jump to charge, then release Down. Applies to "
      "supported Sonic 1 stages only.",
      "Assistance Sonic 1 : maintiens Bas et appuie sur Saut pour charger, puis relâche Bas. "
      "S’applique uniquement aux stages Sonic 1 compatibles.",
      "Ayuda para Sonic 1: mantén Abajo y pulsa salto para cargar; suelta Abajo. Solo en fases "
      "Sonic 1 compatibles.",
      "ソニック1用の補助です。下を押しながらジャンプで溜め、下を離して発進します。対応するソニック1"
      "の場面だけに適用します。"),
    H("fastboot", Assist,
      "Skip supported startup delays and title sequences. Disable for the complete original boot "
      "flow; reset or reload to apply.",
      "Saute les attentes et introductions compatibles. Désactive pour le démarrage original "
      "complet ; relance ou réinitialise pour appliquer.",
      "Salta esperas e introducciones compatibles. Desactívalo para el arranque original completo; "
      "reinicia o recarga para aplicarlo.",
      "対応する起動待ちや導入を省きます。元の起動手順を見る場合は無効にし、リセットまたは再読込して"
      "ください。"),
    H("spritelimit", Performance,
      "Lift the hardware’s sprite-per-line limit to reduce native flicker. Some original visual "
      "effects rely on that limit, so it can be disabled.",
      "Lève la limite matérielle des sprites par ligne pour réduire le clignotement. Certains "
      "effets originaux utilisent cette limite ; tu peux la rétablir.",
      "Quita el límite de sprites por línea para reducir parpadeos. Algunos efectos originales "
      "dependen de él, por lo que puede desactivarse.",
      "1走査線のスプライト制限を緩め、ちらつきを減らします。一部の演出は制限を利用するため、無効に"
      "もできます。"),
    H("rewind", Rewind,
      "Keep a bounded history in memory. Hold the rewind button or Backspace to go back; a reset, "
      "game switch or loaded save starts a new history.",
      "Garde un historique limité en mémoire. Maintiens le bouton de retour ou Retour arrière pour "
      "reculer ; une réinitialisation, un changement de jeu ou un chargement le recommence.",
      "Guarda un historial limitado en memoria. Mantén rebobinar o Retroceso; reiniciar, cambiar "
      "de juego o cargar una partida inicia otro historial.",
      "上限付きの履歴をメモリに記録します。巻き戻しボタンかBackspaceを長押しします。リセット・ゲー"
      "ム変更・ロードで履歴は新しくなります。"),
    H("rewindaudio", Rewind,
      "Play captured sound backwards while rewinding. Disable for silent rewind. Mute and volume "
      "settings still apply.",
      "Lit le son enregistré à l’envers pendant le retour arrière. Désactive pour un retour "
      "silencieux. Muet et volume restent applicables.",
      "Reproduce el sonido grabado al revés al rebobinar. Desactívalo para rebobinar en silencio. "
      "Respeta silencio y volumen.",
      "巻き戻し中に記録した音を逆再生します。無効なら無音です。消音と音量の設定も反映されます。"),
    H("rewindmemory", Rewind,
      "Choose 24, 96 or 192 MB of history memory. Duration depends on game-state compression and "
      "audio; the on-screen hint shows available seconds.",
      "Choisit 24, 96 ou 192 Mo d’historique. La durée dépend de la compression et du son ; l’aide "
      "à l’écran affiche les secondes disponibles.",
      "Elige 24, 96 o 192 MB de historial. La duración depende de la compresión y el audio; la "
      "ayuda muestra los segundos disponibles.",
      "履歴メモリを24・96・192MBから選びます。保持時間は圧縮率と音声で変わり、画面の案内に残り秒数"
      "を表示します。"),
    H("assist", Assist,
      "Reduce damage in supported Sonic platform stages. This modifies gameplay RAM; turn it off "
      "for original difficulty. Other games are unaffected.",
      "Réduit les dégâts dans les stages Sonic compatibles. Modifie la mémoire du jeu ; désactive "
      "pour la difficulté originale. Les autres jeux ne sont pas concernés.",
      "Reduce el daño en fases Sonic compatibles. Modifica la memoria del juego; desactívalo para "
      "la dificultad original. No afecta a otros juegos.",
      "対応するソニックのステージでダメージを抑えます。ゲームRAMを変更する補助です。本来の難易度で"
      "は無効にしてください。他作品には適用しません。"),
    H("air", Assist,
      "Prevent drowning in supported Sonic platform stages. This is a gameplay assist, separate "
      "from display options.",
      "Évite la noyade dans les stages Sonic compatibles. C’est une assistance de jeu indépendante "
      "de l’affichage.",
      "Evita ahogarse en fases Sonic compatibles. Es una ayuda de juego independiente de la "
      "imagen.",
      "対応するソニックのステージで水中の息切れを防ぐ補助です。表示設定とは独立しています。"),
    H("region", Assist,
      "Automatic uses the cartridge region. PAL runs at the original 50 Hz cadence, NTSC near 60 "
      "Hz. Reload the game after changing region.",
      "Automatique utilise la région de la cartouche. PAL tourne à 50 Hz, NTSC près de 60 Hz. "
      "Recharge le jeu après un changement de région.",
      "Automático usa la región del cartucho. PAL funciona a 50 Hz y NTSC cerca de 60 Hz. Recarga "
      "el juego tras cambiarla.",
      "自動はカートリッジの地域を使います。PALは50Hz、NTSCは約60Hzで進行します。変更後はゲームを再"
      "読込してください。"),
    H("confirmactions", Save,
      "Ask before resetting, replacing saves, loading states or changing games. This helps prevent "
      "accidental progress loss with a controller.",
      "Demande confirmation avant réinitialisation, remplacement de sauvegarde, chargement ou "
      "changement de jeu. Évite les pertes accidentelles à la manette.",
      "Pide confirmar reinicios, sustituciones de guardados, cargas y cambios de juego. Evita "
      "perder progreso por un toque accidental.",
      "リセット・セーブ上書き・ロード・ゲーム変更を確認し、パッドの誤操作による進行の消失を防ぎます"
      "。"),
    H("confirmquit", Save,
      "Ask before closing during a game. Automatic resume saving remains controlled by the "
      "separate autosave setting.",
      "Demande confirmation avant de fermer un jeu en cours. La reprise automatique dépend du "
      "réglage de sauvegarde automatique.",
      "Pide confirmar al salir durante una partida. El guardado de reanudación depende del ajuste "
      "de autoguardado.",
      "ゲーム中の終了を確認します。自動再開用セーブは別の自動保存設定に従います。"),
    H("autosave", Save,
      "Save a resume checkpoint when leaving a loaded game. Manual slots remain separate, and "
      "cartridge SRAM is stored locally.",
      "Enregistre un point de reprise en quittant un jeu chargé. Les emplacements manuels restent "
      "séparés et la SRAM reste locale.",
      "Guarda un punto de reanudación al salir de una partida. Los espacios manuales son "
      "independientes y la SRAM se guarda localmente.",
      "ゲームを離れるときに再開位置を保存します。手動スロットは独立し、カートリッジのSRAMもローカル"
      "保存します。"),
    H("save", Save,
      "Write the selected manual slot. An occupied slot can be replaced after confirmation; other "
      "slots are preserved.",
      "Enregistre dans l’emplacement choisi. Un emplacement occupé peut être remplacé après "
      "confirmation ; les autres sont conservés.",
      "Guarda en el espacio elegido. Si está ocupado, se sustituye tras confirmar; conserva los "
      "demás.",
      "選んだ手動スロットに保存します。使用済みの上書きは確認し、他のスロットは保持します。"),
    H("load", Save,
      "Restore the selected manual checkpoint, including the original game state. Rewind history "
      "restarts at the loaded moment.",
      "Restaure le point manuel choisi avec l’état du jeu original. L’historique du retour arrière "
      "recommence à cet instant.",
      "Restaura el punto manual elegido con el estado original del juego. El historial de "
      "rebobinado vuelve a empezar.",
      "選んだ手動セーブからゲーム状態を復元します。巻き戻し履歴はロードした時点から再開します。"),
    H("reset", Save,
      "Restart the cartridge, using the chosen startup and region settings. Manual save slots are "
      "kept.",
      "Redémarre la cartouche avec les réglages de démarrage et de région choisis. Les sauvegardes "
      "manuelles sont conservées.",
      "Reinicia el cartucho con los ajustes de arranque y región elegidos. Conserva los guardados "
      "manuales.",
      "選んだ起動・地域設定でカートリッジを再起動します。手動セーブは保持します。"),
    H("capture", Save,
      "Save a screenshot locally in the app’s screenshots folder. It is never uploaded "
      "automatically.",
      "Enregistre une capture localement dans le dossier screenshots de l’application. Aucun envoi "
      "automatique.",
      "Guarda una captura local en la carpeta screenshots de la aplicación. Nunca la sube "
      "automáticamente.",
      "アプリのscreenshotsフォルダーへ画像を保存します。自動送信は行いません。"),
    H("readyonly", Library,
      "Filter the library to cartridges ready to play. Disable to see missing games and import "
      "requirements.",
      "Filtre la bibliothèque pour les cartouches prêtes à jouer. Désactive pour voir les jeux "
      "manquants et leurs prérequis.",
      "Filtra la biblioteca a los cartuchos listos. Desactívalo para ver juegos ausentes y "
      "requisitos.",
      "遊べるカートリッジだけを一覧表示します。無効にすると不足作品と必要条件も表示します。"),
    H("import", Library,
      "Choose your own cartridge file. The app checks its contents and keeps game data on this "
      "computer; no ROM is included or downloaded.",
      "Choisis ton fichier de cartouche. L’application vérifie son contenu et le garde sur cet "
      "ordinateur ; aucune ROM n’est fournie ni téléchargée.",
      "Elige tu archivo de cartucho. La aplicación comprueba su contenido y lo guarda en este "
      "equipo; no incluye ni descarga ROMs.",
      "自分のカートリッジファイルを選びます。内容を確認してこのPCに保持します。ROMの同梱・ダウンロ"
      "ードはありません。"),
    H("folder", Library,
      "Scan a local folder for supported cartridge files. Existing saves stay separate from "
      "imported game files.",
      "Analyse un dossier local pour trouver les cartouches compatibles. Les sauvegardes restent "
      "séparées des jeux importés.",
      "Busca cartuchos compatibles en una carpeta local. Los guardados permanecen separados de los "
      "archivos importados.",
      "ローカルフォルダーから対応カートリッジを探します。取り込んだゲームとセーブは別々に保持します"
      "。"),
    H("sourcehelp", Library,
      "Use a cartridge dump you are entitled to use and match the listed revision. This "
      "application provides no ROM-download links or copyrighted game assets.",
      "Utilise une copie de cartouche que tu es autorisé à utiliser et la révision indiquée. "
      "L’application ne fournit ni lien de téléchargement de ROM ni assets protégés.",
      "Usa un volcado que tengas derecho a utilizar y la revisión indicada. La aplicación no "
      "ofrece enlaces de ROMs ni recursos protegidos.",
      "利用する権利のある吸い出しデータと指定の版を使ってください。アプリはROMの配布リンクやゲーム"
      "素材を提供しません。"),
};
#undef H
const SettingHelp *settingHelp(const char *key) {
    for (auto alias : {"desktopframescale", "desktopcabinetangle", "desktopcrtstrength",
                       "desktopcrtmask", "desktopsurround"})
        if (!std::strcmp(key, alias))
            return settingHelp("desktoppresentation");
    for (auto alias : {"questhalf", "questdouble", "questscalereset", "questresizespeed"})
        if (!std::strcmp(key, alias))
            return settingHelp("questscale");
    for (auto alias : {"questcabinetheight", "questcabinettilt", "questpinballpreset"})
        if (!std::strcmp(key, alias))
            return settingHelp("questcabinetwidth");
    for (auto alias : {"questcrtmask", "questcrtstrength", "questcrtcurve", "questcrtpreset"})
        if (!std::strcmp(key, alias))
            return settingHelp("questcrt");
    if (!std::strcmp(key, "questsurroundbrightness"))
        return settingHelp("questsurround");
    if (!std::strcmp(key, "questcurvedpreset"))
        return settingHelp("questcurve");
    if (std::strncmp(key, "slot", 4) == 0 && key[4] >= '1' && key[4] <= '9' && !key[5])
        return settingHelp("save");
    for (auto &entry : entries)
        if (!std::strcmp(entry.key, key))
            return &entry;
    return nullptr;
}
const SettingHelp *allSettingHelp(size_t &count) {
    count = sizeof entries / sizeof *entries;
    return entries;
}
} // namespace scu
