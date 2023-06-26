Public Class Launcher

    Private Sub Form1_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Me.Hide()
        ToolStripMenuItem2.Text = Traduce.Txt("load_profile")
        ToolStripMenuItem4.Text = Traduce.Txt("edit_profile")
        ToolStripMenuItem3.Text = Traduce.Txt("new_profile")
        ToolStripMenuItem1.Text = Traduce.Txt("exit")
        CalibratorToolStripMenuItem.Text = Traduce.Txt("calibrator")
    End Sub

    Private Sub ActualizarLista()
        Me.ToolStripMenuItem2.DropDownItems.Clear()
        Me.ToolStripMenuItem4.DropDownItems.Clear()

        'AddHandler Me.ToolStripMenuItem2.DropDownItems.Add(Traduce.Txt("default")).Click, AddressOf LToolStripMenuItem_Click
        'Me.ToolStripMenuItem2.DropDownItems.Add(New ToolStripSeparator)

        Dim di As New System.IO.DirectoryInfo(".")
        Dim fi As System.IO.FileInfo() = di.GetFiles("*.xhp")
        If fi.Length = 0 Then
            ToolStripMenuItem2.Enabled = False
            ToolStripMenuItem4.Enabled = False
        Else
            ToolStripMenuItem2.Enabled = True
            ToolStripMenuItem4.Enabled = True
        End If
        For i As Integer = 0 To fi.Length - 1
            AddHandler Me.ToolStripMenuItem2.DropDownItems.Add(Mid(fi(i).Name, 1, (fi(i).Name.Length - 4))).Click, AddressOf Carga_Click
            AddHandler Me.ToolStripMenuItem4.DropDownItems.Add(Mid(fi(i).Name, 1, (fi(i).Name.Length - 4))).Click, AddressOf Edita_Click
        Next
    End Sub

#Region "Cargas"
    'Private Sub CargarPredeterminado()
    '    Try
    '        System.IO.File.OpenRead("default.pdt").Close()
    '    Catch
    '        Traduce.Msg("error_default_profile_xhp", MsgBoxStyle.Exclamation)
    '        Exit Sub
    '    End Try
    '    Dim nombre As String = New System.IO.DirectoryInfo(".").FullName & "\default"
    '    If Driver.Lanzar(nombre & ".pdt") Then
    '        Traduce.Msg("load_ok", MsgBoxStyle.Information)
    '    End If
    'End Sub

    Private Sub Cargar(ByVal nombre As String)
        Try
            System.IO.File.OpenRead(nombre & ".xhp").Close()
        Catch
            Traduce.Msg("error_open_xhp", MsgBoxStyle.Exclamation)
            Exit Sub
        End Try
        nombre = New System.IO.DirectoryInfo(".").FullName & "\" & nombre
        If Driver.Lanzar(nombre & ".xhp") Then
            Traduce.Msg("load_ok", MsgBoxStyle.Information)
        End If
    End Sub
#End Region

#Region "Eventos"
    Private Sub ContextMenuStrip1_Opening(ByVal sender As System.Object, ByVal e As System.ComponentModel.CancelEventArgs) Handles ContextMenuStrip1.Opening
        ActualizarLista()
    End Sub

    Private Sub ToolStripMenuItem1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ToolStripMenuItem1.Click
        Me.Close()
    End Sub

    Private Sub ToolStripMenuItem3_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles ToolStripMenuItem3.Click
        Try
            Shell("editor.exe", AppWinStyle.NormalFocus)
        Catch
        End Try
    End Sub

    Private Sub CalibratorToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles CalibratorToolStripMenuItem.Click
        Try
            Shell("calibrator.exe", AppWinStyle.NormalFocus)
        Catch
        End Try
    End Sub
    'Private Sub LToolStripMenuItem_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
    '    CargarPredeterminado()
    'End Sub

    Private Sub Carga_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
        Cargar(CType(sender, ToolStripMenuItem).Text)
    End Sub
    Private Sub Edita_Click(ByVal sender As System.Object, ByVal e As System.EventArgs)
        Try
            Shell("editor.exe " & """" & System.Environment.CurrentDirectory & "\" & CType(sender, ToolStripMenuItem).Text & """", AppWinStyle.NormalFocus)
        Catch
        End Try
    End Sub
#End Region

End Class