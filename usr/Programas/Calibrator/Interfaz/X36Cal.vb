Public Class X36Cal
    Private js As HIDData
    Private eje As Byte = 0

    Private Declare Auto Function DescalibrarGameport Lib "directinput.dll" () As Byte
    Private Declare Auto Function CalibrarGameport Lib "directinput.dll" () As Byte
    Private Declare Auto Function DescalibrarHID Lib "directinput.dll" () As Byte
    Private Declare Auto Function CalibrarHID Lib "directinput.dll" () As Byte
    Private Sub X36Cal_Disposed(ByVal sender As Object, ByVal e As System.EventArgs) Handles Me.Disposed
        Timer1.Stop()
        js.Cerrar()
        Dim b As Byte = CalibrarGameport()
        b = CalibrarHID()
    End Sub
    Private Sub X36Cal_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
#If LITE = 1 Then
        GroupBox3.Visible=False 
#End If
        Traducir()
        js = New HIDData(Me.ParentForm)
        Dim b As Byte = DescalibrarGameport()
        If b = 1 Then Traduce.Msg("error_opening_device", MsgBoxStyle.Exclamation)
        If b = 2 Then Traduce.Msg("error_accessing_device", MsgBoxStyle.Exclamation)
        b = DescalibrarHID()
        If b = 1 Then Traduce.Msg("error_opening_device", MsgBoxStyle.Exclamation)
        If b = 2 Then Traduce.Msg("error_accessing_device", MsgBoxStyle.Exclamation)
        Timer1.Start()
    End Sub

    Private Sub Traducir()
        GroupBox1.Text = Traduce.Txt("raw_data")
        Button5.Text = Traduce.Txt("reset_graphic_bar")
        Label3.Text = Traduce.Txt("bottom_limit")
        Label4.Text = Traduce.Txt("center")
        Label5.Text = Traduce.Txt("null_zone")
        Label7.Text = Traduce.Txt("upper_limit")
        GroupBox3.Text = Traduce.Txt("antovibration")
        CheckBox1.Text = Traduce.Txt("activate")
        Label8.Text = Traduce.Txt("resistance")
        Label6.Text = Traduce.Txt("margin")
        GroupBox2.Text = Traduce.Txt("calibration_result")
        Button4.Text = Traduce.Txt("apply")
    End Sub

    Private Sub Timer1_Tick(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Timer1.Tick
        js.Poll()
        PictureBox1.Refresh()
        PictureBox2.Refresh()
    End Sub

    Private Sub CheckBox1_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CheckBox1.CheckedChanged
        If CheckBox1.Checked Then
            TextBox5.Enabled = True
            TextBox6.Enabled = True
        Else
            TextBox5.Enabled = False
            TextBox6.Enabled = False
        End If
    End Sub

    Private Sub RadioButton1_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RadioButton1.CheckedChanged
        If RadioButton1.Checked Then
            eje = 0
            LeerRegistro()
        End If
    End Sub

    Private Sub RadioButton2_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RadioButton2.CheckedChanged
        If RadioButton2.Checked Then
            eje = 1
            LeerRegistro()
        End If
    End Sub

    Private Sub RadioButton3_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RadioButton3.CheckedChanged
        If RadioButton3.Checked Then
            eje = 2
            LeerRegistro()
        End If
    End Sub

    Private Sub RadioButton4_CheckedChanged(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles RadioButton4.CheckedChanged
        If RadioButton4.Checked Then
            eje = 3
            LeerRegistro()
        End If
    End Sub

#Region "Resultado"
    Private Sub PictureBox1_Paint(ByVal sender As Object, ByVal e As System.Windows.Forms.PaintEventArgs) Handles PictureBox1.Paint
        Select Case eje
            Case 0
                Dim pos As Integer = js.X
                PruebaCalibrar(pos, js.limitexy)
                e.Graphics.FillRectangle(Brushes.Red, 0, 0, (pos * 449) \ js.limitexy, 30)
                Label1.Text = pos
            Case 1
                Dim pos As Integer = js.Y
                PruebaCalibrar(pos, js.limitexy)
                e.Graphics.FillRectangle(Brushes.Red, 0, 0, (pos * 449) \ js.limitexy, 30)
                Label1.Text = pos
            Case 2
                Dim pos As Integer = js.R
                PruebaCalibrar(pos, 255)
                e.Graphics.FillRectangle(Brushes.Red, 0, 0, (pos * 449) \ js.limiteR, 30)
                Label1.Text = pos
            Case 3
                Dim pos As Integer = js.Z
                PruebaCalibrar(pos, 255)
                e.Graphics.FillRectangle(Brushes.Red, 0, 0, CInt(pos * (449 / 255)), 30)
                Label1.Text = pos
        End Select
    End Sub

    Dim posElegida As Integer = 0
    Dim posRepetida As Byte = 0
    Private Sub PruebaCalibrar(ByRef pos As Integer, ByVal rango As Integer)
        Dim margen As Byte = 0
        Try
            margen = Byte.Parse(TextBox5.Text)
        Catch
            TextBox5.Text = "0"
        End Try
        Dim resistencia As Byte = 0
        Try
            resistencia = Byte.Parse(TextBox6.Text)
        Catch
            TextBox6.Text = "0"
        End Try
        If CheckBox1.Checked Then
            If margen > 0 Then
                ' Antivibraciones
                If (pos > (posElegida + margen)) Or (pos < (posElegida - margen)) Then
                    posRepetida = 0
                    posElegida = pos
                Else
                    If posRepetida < resistencia Then
                        posRepetida = posRepetida + 1
                        pos = posElegida
                    Else
                        posRepetida = 0
                        posElegida = pos
                    End If
                End If
            End If
        End If

        ' Calibrado
        Dim ancho1, ancho2 As Integer
        Dim c, n, i, d As Integer
        Try
            c = Integer.Parse(TextBox2.Text)
        Catch
            TextBox2.Text = "0"
        End Try
        Try
            n = Integer.Parse(TextBox4.Text)
        Catch
            TextBox4.Text = "0"
        End Try
        Try
            d = Integer.Parse(TextBox3.Text)
        Catch
            TextBox3.Text = "10000"
        End Try
        Try
            i = Integer.Parse(TextBox1.Text)
        Catch
            TextBox1.Text = "0"
        End Try
        ancho1 = (c - n) - i
        ancho2 = d - (c + n)
        If ancho1 = 0 Then ancho1 = 1
        If ancho2 = 0 Then ancho2 = 1
        If pos = 0 Or ((pos >= (c - n)) And (pos <= (c + n))) Then
            pos = rango \ 2
            Exit Sub
        Else
            If pos < i Then
                pos = i
            Else
                If pos > d Then
                    pos = d
                End If
            End If
            If pos < c Then
                pos = (c - n) - pos
                If ancho1 > ancho2 Then
                    pos = (pos * ancho2) / ancho1
                End If
                pos = 0 - pos
            Else
                pos = pos - (c + n)
                If ancho2 > ancho1 Then
                    pos = (pos * ancho1) / ancho2
                End If
            End If
        End If
        ' Pasar a rango hid
        If ancho2 > ancho1 Then
            pos = CInt(((pos + ancho1) * rango) \ (2 * ancho1))
        Else
            pos = CInt(((pos + ancho2) * rango) \ (2 * ancho2))
        End If
    End Sub
#End Region

#Region "raw data"
    Private limitex1 As Integer = &HFFFFFFFF
    Private limitex2 As Integer = 0
    Private limitey1 As Integer = &HFFFFFFFF
    Private limitey2 As Integer = 0
    Private limiter1 As Integer = &HFFFFFFFF
    Private limiter2 As Integer = 0
    Private limitez1 As Integer = &HFFFFFFFF
    Private limitez2 As Integer = 0
    Private Sub Button5_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button5.Click
        limitex1 = &HFFFFFFFF
        limitex2 = 0
        limitey1 = &HFFFFFFFF
        limitey2 = 0
        limiter1 = &HFFFFFFFF
        limiter2 = 0
        limitez1 = &HFFFFFFFF
        limitez2 = 0
    End Sub
    Private Sub PictureBox2_Paint(ByVal sender As Object, ByVal e As System.Windows.Forms.PaintEventArgs) Handles PictureBox2.Paint
        If js.X > limitex2 Then limitex2 = js.X
        If js.X < limitex1 Then limitex1 = js.X
        If js.Y > limitey2 Then limitey2 = js.Y
        If js.Y < limitey1 Then limitey1 = js.Y
        If js.R > limiter2 Then limiter2 = js.R
        If js.R < limiter1 Then limiter1 = js.R
        If js.Z > limitez2 Then limitez2 = js.Z
        If js.Z < limitez1 Then limitez1 = js.Z
        Select Case eje
            Case 0
                If (limitex2 - limitex1) > 0 Then e.Graphics.FillRectangle(Brushes.Red, 0, 0, (js.X * 449) \ (limitex2 - limitex1), 30)
                Label2.Text = js.X
            Case 1
                If (limitey2 - limitey1) > 0 Then e.Graphics.FillRectangle(Brushes.Red, 0, 0, (js.Y * 449) \ (limitey2 - limitey1), 30)
                Label2.Text = js.Y
            Case 2
                If (limiter2 - limiter1) > 0 Then e.Graphics.FillRectangle(Brushes.Red, 0, 0, (js.R * 449) \ (limiter2 - limiter1), 30)
                Label2.Text = js.R
            Case 3
                If (limitez2 - limitez1) > 0 Then e.Graphics.FillRectangle(Brushes.Red, 0, 0, (js.Z * 449) \ (limitez2 - limitez1), 30)
                Label2.Text = js.Z
        End Select
    End Sub
#End Region

    Private Sub Button4_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button4.Click
        Dim li As Integer = 0
        Dim lc As Integer
        Dim ld As Integer
        Dim n As Byte = 0
        Dim vr As Byte = 0
        Dim vm As Byte = 0
        Select Case eje
            Case 0
                lc = js.limitexy \ 2
                ld = js.limitexy
            Case 1
                lc = js.limitexy \ 2
                ld = js.limitexy
            Case 2
                lc = 127
                ld = 255
            Case 3
                lc = 127
                ld = 255
        End Select
        Try
            li = Integer.Parse(TextBox1.Text)
        Catch
            TextBox1.Text = li
        End Try
        Try
            lc = Integer.Parse(TextBox2.Text)
        Catch
            TextBox2.Text = lc
        End Try
        Try
            ld = Integer.Parse(TextBox3.Text)
        Catch
            TextBox3.Text = ld
        End Try
        Try
            n = Byte.Parse(TextBox4.Text)
        Catch
            TextBox4.Text = n
        End Try
        Try
            vm = Integer.Parse(TextBox5.Text)
        Catch
            TextBox5.Text = vm
        End Try
        Try
            vr = Integer.Parse(TextBox6.Text)
        Catch
            TextBox6.Text = vr
        End Try
        If Me.CheckBox1.Checked Then
            Calibrar(li, lc, ld, n, 1, vm, vr)
        Else
            Calibrar(li, lc, ld, n, 0, vm, vr)
        End If
    End Sub

    Private Structure CALDATA
        Public i As UInt32
        Public c As UInt32
        Public d As UInt32
        Public n As Byte
        Public vm As Byte
        Public vr As Byte
        Public cal As Byte
        Public v As Byte
    End Structure
    Private Declare Auto Function EscribirCalibradoGameport Lib "directinput.dll" () As Byte
    Private Sub Calibrar(ByVal i As UInt32, ByVal c As UInt32, ByVal d As UInt32, ByVal n As Byte, ByVal av As Byte, ByVal vm As Byte, ByVal vr As Byte)
        Dim reg As Microsoft.Win32.RegistryKey = Nothing
        Dim st As CALDATA
        st.i = i
        st.c = c
        st.d = d
        st.n = n
        st.v = av
        st.vm = vm
        st.vr = vr
        st.cal = 1
        Dim gch As Runtime.InteropServices.GCHandle = Runtime.InteropServices.GCHandle.Alloc(st, Runtime.InteropServices.GCHandleType.Pinned)
        Dim buff(19) As Byte
        Runtime.InteropServices.Marshal.Copy(gch.AddrOfPinnedObject, buff, 0, 17)
        gch.Free()
        Try
            Microsoft.Win32.Registry.LocalMachine.CreateSubKey("SYSTEM\CurrentControlSet\Control\MediaProperties\PrivateProperties\Joystick\Calibrado")
            reg = Microsoft.Win32.Registry.LocalMachine.OpenSubKey("SYSTEM\CurrentControlSet\Control\MediaProperties\PrivateProperties\Joystick\Calibrado", True)
            reg.SetValue("GameportEje" & (eje + 1), buff, Microsoft.Win32.RegistryValueKind.Binary)
            reg.Close()
        Catch ex As Exception
            If reg IsNot Nothing Then reg.Close()
            MsgBox(ex.Message, MsgBoxStyle.Exclamation, "XCalibrator")
            Exit Sub
        End Try
        Select Case EscribirCalibradoGameport()
            Case 1
                Traduce.Msg("error_opening_device", MsgBoxStyle.Exclamation)
            Case 2
                Traduce.Msg("error_accessing_device", MsgBoxStyle.Exclamation)
            Case Else
                Traduce.Msg("configuration_saved", MsgBoxStyle.Exclamation)
        End Select
    End Sub

    Private Sub LeerRegistro()
        Dim reg As Microsoft.Win32.RegistryKey = Nothing
        Dim buff(20) As Byte
        Try
            reg = Microsoft.Win32.Registry.LocalMachine.OpenSubKey("SYSTEM\CurrentControlSet\Control\MediaProperties\PrivateProperties\Joystick\Calibrado", False)
            If reg IsNot Nothing Then
                buff = reg.GetValue("GameportEje" & (eje + 1))
                reg.Close()
            End If
        Catch ex As Exception
            If reg IsNot Nothing Then reg.Close()
            MsgBox(ex.Message, MsgBoxStyle.Exclamation, "XCalibrator")
            Exit Sub
        End Try

        Dim st As CALDATA
        Dim gch As Runtime.InteropServices.GCHandle = Runtime.InteropServices.GCHandle.Alloc(buff, Runtime.InteropServices.GCHandleType.Pinned)
        st = Runtime.InteropServices.Marshal.PtrToStructure(gch.AddrOfPinnedObject, st.GetType())
        gch.Free()

        TextBox1.Text = st.i
        TextBox2.Text = st.c
        If (st.d = 0) Then TextBox3.Text = "10000" Else TextBox3.Text = st.d
        TextBox4.Text = st.n
        TextBox5.Text = st.vm
        TextBox6.Text = st.vr
        If st.v = 1 Then Me.CheckBox1.Checked = True Else Me.CheckBox1.Checked = False
    End Sub
End Class
