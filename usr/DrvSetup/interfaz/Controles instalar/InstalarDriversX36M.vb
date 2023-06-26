Public Class InstalarDriversX36M
    Private conError As Boolean = False
    Private terminado As Boolean = False

    Public Sub Procesar(ByVal selTeclado As Integer)
        Me.ParentForm.Hide()
        If IO.File.Exists("drvapi.exe") Then
            Dim newProc As Diagnostics.Process = Diagnostics.Process.Start("drvapi.exe", "2 " & selTeclado & " 0")
            newProc.WaitForExit()
            Select Case newProc.ExitCode
                Case 1
                    Me.PictureBox1.Image = My.Resources.OK
                Case 2
                    Me.PictureBox1.Image = My.Resources.OK
                    Me.PictureBox2.Image = My.Resources.OK
                Case 3
                    Me.PictureBox1.Image = My.Resources.OK
                    Me.PictureBox2.Image = My.Resources.OK
                    Me.PictureBox5.Image = My.Resources.OK
                    Me.ParentForm.DialogResult = DialogResult.OK
                    Threading.Thread.Sleep(2000)
                    Me.ParentForm.Close()
                    Exit Sub
            End Select
        End If
        Me.ParentForm.Show()
        Me.Button1.Visible = True
    End Sub

    Private Sub Button1_Click(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles Button1.Click
        Me.ParentForm.Close()
    End Sub

    Private Sub InstalarDriversX36G_Load(ByVal sender As System.Object, ByVal e As System.EventArgs) Handles MyBase.Load
        Select Case (System.Globalization.CultureInfo.CurrentCulture.LCID And &HFF)
            Case &HC
                'nombre = "french.xml"
            Case &H10
                'nombre = "italian.xml"
            Case &HA
                Label1.Text = "Filtro controlador de teclado X36"
                Label2.Text = "Controlador de pasarela de teclado X36 a HID"
                Label5.Text = "Controlador XHOTAS Virtual de teclado y ratón"
                Button1.Text = "Finalizar"
            Case &H7
                'nombre = "german.xml"
        End Select
    End Sub
End Class
