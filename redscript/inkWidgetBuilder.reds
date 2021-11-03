public class inkWidgetBuilder {
  private let widget: ref<inkWidget>;

  // inkWidget generics

  public func Visible(value: Bool) -> ref<inkWidgetBuilder> {
    this.widget.SetVisible(value);
    return this;
  }

  public func Reparent(parent: ref<inkCompoundWidget>) -> ref<inkWidgetBuilder> {
    this.widget.Reparent(parent);
    return this;
  }

  public func Tint(tint: HDRColor) -> ref<inkWidgetBuilder> {
    this.widget.SetTintColor(tint);
    return this;
  }

  public func Opacity(opacity: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetOpacity(opacity);
    return this;
  }

  public func Anchor(anchor: inkEAnchor) -> ref<inkWidgetBuilder> {
    this.widget.SetAnchor(anchor);
    return this;
  }

  public func Anchor(anchorPoint: Vector2) -> ref<inkWidgetBuilder> {
    this.widget.SetAnchorPoint(anchorPoint);
    return this;
  }

  public func Anchor(x: Float, y: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetAnchorPoint(x, y);
    return this;
  }

  public func Size(x: Float, y: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetSize(x, y);
    return this;
  }

  public func Margin(margin: inkMargin) -> ref<inkWidgetBuilder> {
    this.widget.SetMargin(margin);
    return this;
  }

  public func Margin(a: Float, b: Float, c: Float, d: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetMargin(a, b, c, d);
    return this;
  }

  public func Padding(padding: inkMargin) -> ref<inkWidgetBuilder> {
    this.widget.SetPadding(padding);
    return this;
  }

  public func Padding(a: Float, b: Float, c: Float, d: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetPadding(a, b, c, d);
    return this;
  }

  public func FitToContent(fit: Bool) -> ref<inkWidgetBuilder> {
    this.widget.SetFitToContent(fit);
    return this;
  }

  public func Scale(x: Float, y: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetScale(new Vector2(x, y));
    return this;
  }

  public func Scale(scale: Vector2) -> ref<inkWidgetBuilder> {
    this.widget.SetScale(scale);
    return this;
  }

  public func Translation(Translation: Vector2) -> ref<inkWidgetBuilder> {
    this.widget.SetTranslation(Translation);
    return this;
  }

  public func Translation(x: Float, y: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetTranslation(x, y);
    return this;
  }

  public func Rotation(Rotation: Float) -> ref<inkWidgetBuilder> {
    this.widget.SetRotation(Rotation);
    return this;
  }

  // inkCanvas methods

  public static func inkCanvas(name: CName) -> ref<inkWidgetBuilder> {
    let instance = new inkWidgetBuilder();
    instance.widget = new inkCanvas() as inkWidget;
    instance.widget.SetName(name);
    return instance;
  }

  public func BuildCanvas() -> ref<inkCanvas> {
    return this.widget as inkCanvas;
  }

  // inkFlex methods

  public static func inkFlex(name: CName) -> ref<inkWidgetBuilder> {
    let instance = new inkWidgetBuilder();
    instance.widget = new inkFlex() as inkWidget;
    instance.widget.SetName(name);
    return instance;
  }

  public func BuildFlex() -> ref<inkFlex> {
    return this.widget as inkFlex;
  }

  // inkImage methods

  public static func inkImage(name: CName) -> ref<inkWidgetBuilder> {
    let instance = new inkWidgetBuilder();
    instance.widget = new inkImage() as inkWidget;
    instance.widget.SetName(name);
    return instance;
  }

  public func BuildImage() -> ref<inkImage> {
    return this.widget as inkImage;
  }

  public func Atlas(atlas: ResRef) -> ref<inkWidgetBuilder> {
    (this.widget as inkImage).SetAtlasResource(atlas);
    return this;
  }

  public func Part(texture: CName) -> ref<inkWidgetBuilder> {
    (this.widget as inkImage).SetTexturePart(texture);
    return this;
  }

  public func NineSliceScale(value: Bool) -> ref<inkWidgetBuilder> {
    (this.widget as inkImage).SetNineSliceScale(value);
    return this;
  }

  // inkText methods

  public static func inkText(name: CName) -> ref<inkWidgetBuilder> {
    let instance = new inkWidgetBuilder();
    instance.widget = new inkText() as inkWidget;
    instance.widget.SetName(name);
    return instance;
  }

  public func BuildText() -> ref<inkText> {
    return this.widget as inkText;
  }

  public func Font(font: String, opt fontStyle: CName) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetFontFamily(font, fontStyle);
    return this;
  }

  public func FontStyle(font: CName) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetFontStyle(font);
    return this;
  }

  public func FontSize(size: Int32) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetFontSize(size);
    return this;
  }

  public func Text(text: String) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetText(text);
    return this;
  }

  public func LetterCase(value: textLetterCase) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetLetterCase(value);
    return this;
  }

  public func Overflow(value: textOverflowPolicy) -> ref<inkWidgetBuilder> {
    (this.widget as inkText).SetOverflowPolicy(value);
    return this;
  }



  // inkRectangle
  
  public static func inkRectangle(name: CName) -> ref<inkWidgetBuilder> {
    let instance = new inkWidgetBuilder();
    instance.widget = new inkRectangle() as inkWidget;
    instance.widget.SetName(name);
    return instance;
  }

  public func BuildRectangle() -> ref<inkRectangle> {
    return this.widget as inkRectangle;
  }

  // inkCircle
  
  public static func inkCircle(name: CName) -> ref<inkWidgetBuilder> {
    let instance = new inkWidgetBuilder();
    instance.widget = new inkCircle() as inkWidget;
    instance.widget.SetName(name);
    return instance;
  }

  public func BuildCircle() -> ref<inkCircle> {
    return this.widget as inkCircle;
  }

  // inkShape
  
  public static func inkShape(name: CName) -> ref<inkWidgetBuilder> {
    let instance = new inkWidgetBuilder();
    instance.widget = new inkShape() as inkWidget;
    instance.widget.SetName(name);
    return instance;
  }

  public func BuildShape() -> ref<inkShape> {
    return this.widget as inkShape;
  }
  
  public func ShapeVariant(shapeVariant: inkEShapeVariant) -> ref<inkWidgetBuilder> {
    (this.widget as inkShape).SetShapeVariant(shapeVariant);
    return this;
  }

  public func BorderOpacity(borderOpacity: Float) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetBorderOpacity(borderOpacity);
    return this;
  }

  public func BorderColor(borderColor: HDRColor) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetBorderColor(borderColor);
    return this;
  }

  public func FillOpacity(fillOpacity: Float) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetFillOpacity(fillOpacity);
    return this;
  }

  public func VertexList(vertexList: array<Vector2>) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetVertexList(vertexList);
    return this;
  }

  public func JointStyle(jointStyle: inkEJointStyle) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetJointStyle(jointStyle);
    return this;
  }

  public func EndCapStyle(endCapStyle: inkEEndCapStyle) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetEndCapStyle(endCapStyle);
    return this;
  }

  public func LineThickness(lineThickness: Float) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetLineThickness(lineThickness);
    return this;
  }
  public func ShapeName(shapeName: CName) -> ref<inkWidgetBuilder>  {
    (this.widget as inkShape).SetShapeName(shapeName);
    return this;
  }


}