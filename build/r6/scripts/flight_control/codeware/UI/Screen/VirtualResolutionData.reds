// -----------------------------------------------------------------------------
// Codeware.UI.VirtualResolutionData
// -----------------------------------------------------------------------------

module Codeware.UI

public class VirtualResolutionData {
	protected let m_resolution: String;

	protected let m_size: Vector2;

	protected let m_scale: Vector2;

	public func GetResolution() -> String {
		return this.m_resolution;
	}

	public func GetSize() -> Vector2 {
		return this.m_size;
	}

	public func GetWidth() -> Float {
		return this.m_size.X;
	}

	public func GetHeight() -> Float {
		return this.m_size.Y;
	}

	public func GetAspectRatio() -> Float {
		return this.m_size.X / this.m_size.Y;
	}

	public func GetScale() -> Vector2 {
		return this.m_scale;
	}

	public func GetScaleX() -> Float {
		return this.m_scale.X;
	}

	public func GetScaleY() -> Float {
		return this.m_scale.Y;
	}

	public func GetSmartScaleFactor() -> Float {
		return this.m_scale.X < this.m_scale.Y ? this.m_scale.X : this.m_scale.Y;
	}

	public func GetSmartScale() -> Vector2 {
		let factor: Float = this.GetSmartScaleFactor();

		return new Vector2(factor, factor);
	}

	public static func Create(resolution: String, size: Vector2, scale: Vector2) -> ref<VirtualResolutionData> {
		let data = new VirtualResolutionData();
		data.m_resolution = resolution;
		data.m_size = size;
		data.m_scale = scale;

		return data;
	}
}
