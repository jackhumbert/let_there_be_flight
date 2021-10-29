// public class FlightHUDLogicController extends inkLogicController {

//   private let m_widgetPos: wref<inkText>;

//   private let m_worldPos: wref<inkText>;

//   private let m_projection: ref<inkScreenProjection>;

//   protected cb func OnInitialize() -> Bool {
//     LogChannel(n"DEBUG", "FlightHUDLogicController init");
//     this.GetRootWidget().SetAnchorPoint(new Vector2(0.50, 0.50));
//     this.m_widgetPos = this.GetWidget(n"widgetPos") as inkText;
//     this.m_worldPos = this.GetWidget(n"worldPos") as inkText;
//     this.PlayAnimation();
//   }

//   public final func GetProjection() -> ref<inkScreenProjection> {
//     return this.m_projection;
//   }

//   public final func SetProjection(projection: ref<inkScreenProjection>) -> Void {
//     this.m_projection = projection;
//   }

//   public final func UpdatewidgetPosition(projection: ref<inkScreenProjection>) -> Void {
//     let margin: inkMargin;
//     let rootWidget: wref<inkWidget> = this.GetRootWidget();
//     let gameEntity: ref<GameEntity> = projection.GetEntity() as GameEntity;
//     let widgetPosition: Vector2 = projection.currentPosition;
//     let worldPosition: Vector4 = gameEntity.GetWorldPosition() + projection.GetFixedWorldOffset();
//     margin.left = widgetPosition.X;
//     margin.top = widgetPosition.Y;
//     rootWidget.SetMargin(margin);
//     this.m_widgetPos.SetText("Screen: (" + widgetPosition.X + "," + widgetPosition.Y + ")");
//     this.m_worldPos.SetText("World: (" + worldPosition.X + "," + worldPosition.Y + "," + worldPosition.Z + ")");
//     rootWidget.SetVisible(projection.IsInScreen());
//   }

//   private final func PlayAnimation() -> Void {
//     let animProxy: ref<inkAnimProxy>;
//     let anim: ref<inkAnimDef> = new inkAnimDef();
//     let alphaInterpolator: ref<inkAnimTransparency> = new inkAnimTransparency();
//     alphaInterpolator.SetStartTransparency(1.00);
//     alphaInterpolator.SetEndTransparency(0.00);
//     alphaInterpolator.SetDuration(3.00);
//     alphaInterpolator.SetType(inkanimInterpolationType.Linear);
//     alphaInterpolator.SetMode(inkanimInterpolationMode.EasyIn);
//     anim.AddInterpolator(alphaInterpolator);
//     animProxy = this.GetRootWidget().PlayAnimation(anim);
//     animProxy.RegisterToCallback(inkanimEventType.OnFinish, this, n"OnAnimFinished");
//   }

//   protected cb func OnAnimFinished(proxy: ref<inkAnimProxy>) -> Bool {
//     this.CallCustomCallback(n"OnReadyToRemove");
//   }
// }